// SPDX-License-Identifier: GPL-2.0
/*
 * buzzer_drv.c – Passive Buzzer Character Device Driver (platform_driver)
 *
 * Hardware: KY-006 Passive Buzzer Module
 *           GPIO 13 (BCM) → Hardware PWM1 (RP1, pwmchip0 channel 1)
 *           Rated oscillation range: 1500 ~ 2500 Hz
 *
 * Requires Device Tree overlay "buzzer" which creates a "clockos,buzzer"
 * platform node with a "pwms" property.  See buzzer-overlay.dts.
 *
 * config.txt:
 *   dtoverlay=pwm-2chan,pin2=13,func2=4   # mux GPIO 13 → RP1 PWM1
 *   dtoverlay=buzzer                       # register platform device
 *
 * Interface: /dev/buzzer  (character device)
 *   ioctl BUZZER_SET_FREQ  – set frequency in Hz
 *   ioctl BUZZER_START     – start continuous tone
 *   ioctl BUZZER_STOP      – stop tone
 *   ioctl BUZZER_BEEP      – beep for N ms then auto-stop
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/pwm.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/slab.h>

#include "buzzer_drv.h"

#define DRIVER_NAME "buzzer"
#define CLASS_NAME  "buzzer"

/* ---------- per-device private data -------------------------------------- */

struct buzzer_dev {
	struct pwm_device  *pwm;
	struct cdev         cdev;
	struct class       *cls;
	struct device      *chrdev;
	dev_t               devno;

	unsigned int        freq_hz;
	bool                running;

	struct delayed_work beep_stop_work;
	struct mutex        lock;
};

/* ---------- helpers ------------------------------------------------------- */

/**
 * _apply_freq() – push frequency to PWM hardware.
 * Caller must hold bd->lock.
 */
static int _apply_freq(struct buzzer_dev *bd, unsigned int hz)
{
	unsigned long period_ns;
	struct pwm_state state;

	if (hz < BUZZER_FREQ_MIN)
		hz = BUZZER_FREQ_MIN;
	if (hz > BUZZER_FREQ_MAX)
		hz = BUZZER_FREQ_MAX;

	bd->freq_hz = hz;
	period_ns   = 1000000000UL / hz;

	pwm_get_state(bd->pwm, &state);
	state.period     = period_ns;
	state.duty_cycle = period_ns / 2;   /* 50 % duty cycle */
	state.polarity   = PWM_POLARITY_NORMAL;
	/* enabled flag unchanged – caller decides */

	return pwm_apply_might_sleep(bd->pwm, &state);
}

/* ---------- delayed-work handler (auto-stop after BUZZER_BEEP) ----------- */

static void beep_stop_fn(struct work_struct *work)
{
	struct buzzer_dev *bd =
		container_of(work, struct buzzer_dev, beep_stop_work.work);
	struct pwm_state state;

	mutex_lock(&bd->lock);
	pwm_get_state(bd->pwm, &state);
	state.enabled = false;
	pwm_apply_might_sleep(bd->pwm, &state);
	bd->running = false;
	mutex_unlock(&bd->lock);
}

/* ---------- file operations ----------------------------------------------- */

static int buzzer_open(struct inode *inode, struct file *file)
{
	/* Recover buzzer_dev from the embedded cdev */
	struct buzzer_dev *bd = container_of(inode->i_cdev,
					     struct buzzer_dev, cdev);
	file->private_data = bd;
	return 0;
}

static int buzzer_release(struct inode *inode, struct file *file)
{
	return 0;
}

static long buzzer_ioctl(struct file *file,
			 unsigned int cmd, unsigned long arg)
{
	struct buzzer_dev *bd  = file->private_data;
	unsigned int       val = 0;
	struct pwm_state   state;
	int                ret = 0;

	if (_IOC_TYPE(cmd) != BUZZER_IOC_MAGIC)
		return -ENOTTY;

	/*
	 * cancel_delayed_work_sync() must be called WITHOUT holding bd->lock,
	 * because beep_stop_fn() also acquires bd->lock.  Calling _sync()
	 * inside the lock would deadlock if the work is currently running.
	 */
	if (cmd == BUZZER_START || cmd == BUZZER_STOP || cmd == BUZZER_BEEP)
		cancel_delayed_work_sync(&bd->beep_stop_work);

	mutex_lock(&bd->lock);

	switch (cmd) {

	/* ---- BUZZER_SET_FREQ ---- */
	case BUZZER_SET_FREQ:
		if (copy_from_user(&val, (unsigned int __user *)arg, sizeof(val))) {
			ret = -EFAULT;
			break;
		}
		ret = _apply_freq(bd, val);
		if (!ret)
			pr_info("buzzer: freq → %u Hz\n", bd->freq_hz);
		break;

	/* ---- BUZZER_START ---- */
	case BUZZER_START:
		pwm_get_state(bd->pwm, &state);
		state.enabled = true;
		ret = pwm_apply_might_sleep(bd->pwm, &state);
		if (!ret)
			bd->running = true;
		break;

	/* ---- BUZZER_STOP ---- */
	case BUZZER_STOP:
		pwm_get_state(bd->pwm, &state);
		state.enabled = false;
		pwm_apply_might_sleep(bd->pwm, &state);
		bd->running = false;
		break;

	/* ---- BUZZER_BEEP ---- */
	case BUZZER_BEEP:
		if (copy_from_user(&val, (unsigned int __user *)arg, sizeof(val))) {
			ret = -EFAULT;
			break;
		}
		if (val == 0 || val > 30000) {
			ret = -EINVAL;
			break;
		}
		pwm_get_state(bd->pwm, &state);
		state.enabled = true;
		ret = pwm_apply_might_sleep(bd->pwm, &state);
		if (!ret) {
			bd->running = true;
			schedule_delayed_work(&bd->beep_stop_work,
					      msecs_to_jiffies(val));
		}
		break;

	default:
		ret = -ENOTTY;
	}

	mutex_unlock(&bd->lock);
	return ret;
}

static const struct file_operations buzzer_fops = {
	.owner          = THIS_MODULE,
	.open           = buzzer_open,
	.release        = buzzer_release,
	.unlocked_ioctl = buzzer_ioctl,
};

/* ---------- platform_driver probe / remove -------------------------------- */

static int buzzer_probe(struct platform_device *pdev)
{
	struct buzzer_dev *bd;
	struct pwm_state   state;
	int ret;

	bd = devm_kzalloc(&pdev->dev, sizeof(*bd), GFP_KERNEL);
	if (!bd)
		return -ENOMEM;

	/*
	 * devm_pwm_get() reads the "pwms" DT property and requests the
	 * PWM channel described there.  The PWM is automatically released
	 * when the platform device is unbound (after remove() returns).
	 *
	 * Returns -EPROBE_DEFER if the PWM provider is not ready yet,
	 * causing the kernel to retry probe() later.
	 */
	bd->pwm = devm_pwm_get(&pdev->dev, NULL);
	if (IS_ERR(bd->pwm)) {
		ret = PTR_ERR(bd->pwm);
		if (ret != -EPROBE_DEFER)
			dev_err(&pdev->dev, "devm_pwm_get failed: %d\n", ret);
		return ret;
	}

	mutex_init(&bd->lock);
	INIT_DELAYED_WORK(&bd->beep_stop_work, beep_stop_fn);
	bd->freq_hz = BUZZER_FREQ_DEFAULT;
	bd->running = false;

	/* Configure default frequency, output disabled */
	pwm_init_state(bd->pwm, &state);
	state.period     = 1000000000UL / BUZZER_FREQ_DEFAULT;
	state.duty_cycle = state.period / 2;
	state.polarity   = PWM_POLARITY_NORMAL;
	state.enabled    = false;
	ret = pwm_apply_might_sleep(bd->pwm, &state);
	if (ret) {
		dev_err(&pdev->dev, "initial pwm_apply failed: %d\n", ret);
		return ret;
	}

	/* Register character device */
	ret = alloc_chrdev_region(&bd->devno, 0, 1, DRIVER_NAME);
	if (ret) {
		dev_err(&pdev->dev, "alloc_chrdev_region failed: %d\n", ret);
		return ret;
	}

	cdev_init(&bd->cdev, &buzzer_fops);
	bd->cdev.owner = THIS_MODULE;
	ret = cdev_add(&bd->cdev, bd->devno, 1);
	if (ret) {
		dev_err(&pdev->dev, "cdev_add failed: %d\n", ret);
		goto err_region;
	}

	bd->cls = class_create(CLASS_NAME);
	if (IS_ERR(bd->cls)) {
		ret = PTR_ERR(bd->cls);
		goto err_cdev;
	}

	bd->chrdev = device_create(bd->cls, &pdev->dev,
				   bd->devno, NULL, DRIVER_NAME);
	if (IS_ERR(bd->chrdev)) {
		ret = PTR_ERR(bd->chrdev);
		goto err_class;
	}

	platform_set_drvdata(pdev, bd);

	dev_info(&pdev->dev, "loaded → /dev/%s  default=%u Hz\n",
		 DRIVER_NAME, bd->freq_hz);
	return 0;

err_class:
	class_destroy(bd->cls);
err_cdev:
	cdev_del(&bd->cdev);
err_region:
	unregister_chrdev_region(bd->devno, 1);
	return ret;
}

static int buzzer_remove(struct platform_device *pdev)
{
	struct buzzer_dev *bd = platform_get_drvdata(pdev);
	struct pwm_state   state;

	cancel_delayed_work_sync(&bd->beep_stop_work);

	/* Stop PWM before devres releases it */
	pwm_get_state(bd->pwm, &state);
	state.enabled = false;
	pwm_apply_might_sleep(bd->pwm, &state);

	device_destroy(bd->cls, bd->devno);
	class_destroy(bd->cls);
	cdev_del(&bd->cdev);
	unregister_chrdev_region(bd->devno, 1);

	dev_info(&pdev->dev, "unloaded\n");
	return 0;
}

/* ---------- driver registration ------------------------------------------ */

static const struct of_device_id buzzer_of_match[] = {
	{ .compatible = "clockos,buzzer" },
	{ }
};
MODULE_DEVICE_TABLE(of, buzzer_of_match);

static struct platform_driver buzzer_driver = {
	.probe  = buzzer_probe,
	.remove = buzzer_remove,
	.driver = {
		.name           = DRIVER_NAME,
		.of_match_table = buzzer_of_match,
	},
};
module_platform_driver(buzzer_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("clockOS");
MODULE_DESCRIPTION("Passive Buzzer Character Device Driver – GPIO 13 / PWM1");
MODULE_VERSION("1.1");
