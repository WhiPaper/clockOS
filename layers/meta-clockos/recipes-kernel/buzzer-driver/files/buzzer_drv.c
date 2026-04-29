// SPDX-License-Identifier: GPL-2.0
/*
 * buzzer_drv.c – Passive Buzzer Character Device Driver
 *
 * Hardware: KY-006 Passive Buzzer Module
 *           GPIO 13 (BCM) wired to signal pin, hardware PWM1 (RP1).
 *           Rated oscillation range: 1500 ~ 2500 Hz
 *
 * Interface: /dev/buzzer  (character device)
 *   ioctl BUZZER_SET_FREQ  – set frequency in Hz
 *   ioctl BUZZER_START     – start continuous tone
 *   ioctl BUZZER_STOP      – stop tone
 *   ioctl BUZZER_BEEP      – beep for N ms then auto-stop
 *
 * Module parameters:
 *   pwm_id  (int, default 1) – global PWM index
 *                              On RPi 5 with dtoverlay=pwm-2chan,pin2=13,func2=4
 *                              PWM1 is typically index 1.  Adjust if needed and
 *                              verify with: ls /sys/class/pwm/
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/pwm.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/slab.h>

#include "buzzer_drv.h"

/* ---------- module parameters -------------------------------------------- */

static int pwm_id = 1;
module_param(pwm_id, int, 0444);
MODULE_PARM_DESC(pwm_id, "Global PWM index for GPIO 13 / PWM1 (default: 1)");

struct rp1_pwm_compat {
	struct pwm_chip chip;
};

/* ---------- driver private data ------------------------------------------ */

#define DRIVER_NAME "buzzer"
#define CLASS_NAME  "buzzer"

struct buzzer_dev {
	struct pwm_device  *pwm;
	struct cdev         cdev;
	struct class       *cls;
	struct device      *dev;
	dev_t               devno;

	unsigned int        freq_hz;
	bool                running;

	struct delayed_work beep_stop_work;
	struct mutex        lock;
};

static struct buzzer_dev g_bdev;

/* ---------- helpers ------------------------------------------------------- */

/**
 * _apply_freq() – push current frequency to the PWM hardware.
 *
 * Caller must hold g_bdev.lock.
 */
static int _apply_freq(struct buzzer_dev *bd, unsigned int hz)
{
	unsigned long period_ns, duty_ns;
	struct pwm_state state;

	if (hz < BUZZER_FREQ_MIN)
		hz = BUZZER_FREQ_MIN;
	if (hz > BUZZER_FREQ_MAX)
		hz = BUZZER_FREQ_MAX;

	bd->freq_hz = hz;
	period_ns   = 1000000000UL / hz;
	duty_ns     = period_ns / 2;   /* 50 % duty cycle */

	pwm_get_state(bd->pwm, &state);
	state.period     = period_ns;
	state.duty_cycle = duty_ns;
	state.polarity   = PWM_POLARITY_NORMAL;
	/* keep enabled flag as-is; caller decides whether to enable */

	return pwm_apply_might_sleep(bd->pwm, &state);
}

static struct pwm_device *buzzer_request_pwm(unsigned int global_pwm_id)
{
	struct device_node *np;
	struct platform_device *pdev;
	struct rp1_pwm_compat *rp1;
	struct pwm_device *pwm;
	int local_pwm_id;

	np = of_find_compatible_node(NULL, NULL, "raspberrypi,rp1-pwm");
	if (!np)
		return ERR_PTR(-ENODEV);

	pdev = of_find_device_by_node(np);
	of_node_put(np);
	if (!pdev)
		return ERR_PTR(-ENODEV);

	rp1 = platform_get_drvdata(pdev);
	if (!rp1) {
		put_device(&pdev->dev);
		return ERR_PTR(-ENODEV);
	}

	if ((int)global_pwm_id < rp1->chip.base ||
	    (int)global_pwm_id >= rp1->chip.base + (int)rp1->chip.npwm) {
		put_device(&pdev->dev);
		return ERR_PTR(-EINVAL);
	}

	local_pwm_id = global_pwm_id - rp1->chip.base;
	pwm = pwm_request_from_chip(&rp1->chip, local_pwm_id, DRIVER_NAME);
	put_device(&pdev->dev);

	return pwm;
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
	file->private_data = &g_bdev;
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
		cancel_delayed_work(&bd->beep_stop_work);
		pwm_get_state(bd->pwm, &state);
		state.enabled = true;
		ret = pwm_apply_might_sleep(bd->pwm, &state);
		if (!ret)
			bd->running = true;
		break;

	/* ---- BUZZER_STOP ---- */
	case BUZZER_STOP:
		cancel_delayed_work(&bd->beep_stop_work);
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
		cancel_delayed_work(&bd->beep_stop_work);
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

/* ---------- module init / exit ------------------------------------------- */

static int __init buzzer_init(void)
{
	struct buzzer_dev *bd = &g_bdev;
	struct pwm_state   state;
	int ret;

	/* Request PWM channel by global index */
	bd->pwm = buzzer_request_pwm(pwm_id);
	if (IS_ERR(bd->pwm)) {
		pr_err("buzzer: pwm request(%d) failed: %ld\n",
		       pwm_id, PTR_ERR(bd->pwm));
		pr_err("buzzer: ensure dtoverlay=pwm-2chan,pin2=13,func2=4 "
		       "is active, or pass pwm_id=<N> matching your PWM chip\n");
		return PTR_ERR(bd->pwm);
	}

	mutex_init(&bd->lock);
	INIT_DELAYED_WORK(&bd->beep_stop_work, beep_stop_fn);
	bd->freq_hz = BUZZER_FREQ_DEFAULT;
	bd->running = false;

	/* Configure default frequency, keep disabled */
	pwm_init_state(bd->pwm, &state);
	state.period     = 1000000000UL / BUZZER_FREQ_DEFAULT;
	state.duty_cycle = state.period / 2;
	state.polarity   = PWM_POLARITY_NORMAL;
	state.enabled    = false;
	ret = pwm_apply_might_sleep(bd->pwm, &state);
	if (ret) {
		pr_err("buzzer: initial pwm_apply failed: %d\n", ret);
		goto err_pwm;
	}

	/* Allocate chrdev region */
	ret = alloc_chrdev_region(&bd->devno, 0, 1, DRIVER_NAME);
	if (ret) {
		pr_err("buzzer: alloc_chrdev_region failed: %d\n", ret);
		goto err_pwm;
	}

	cdev_init(&bd->cdev, &buzzer_fops);
	bd->cdev.owner = THIS_MODULE;
	ret = cdev_add(&bd->cdev, bd->devno, 1);
	if (ret) {
		pr_err("buzzer: cdev_add failed: %d\n", ret);
		goto err_region;
	}

	bd->cls = class_create(CLASS_NAME);
	if (IS_ERR(bd->cls)) {
		ret = PTR_ERR(bd->cls);
		goto err_cdev;
	}

	bd->dev = device_create(bd->cls, NULL, bd->devno, NULL, DRIVER_NAME);
	if (IS_ERR(bd->dev)) {
		ret = PTR_ERR(bd->dev);
		goto err_class;
	}

	pr_info("buzzer: loaded → /dev/%s  pwm_id=%d  default=%u Hz\n",
		DRIVER_NAME, pwm_id, bd->freq_hz);
	return 0;

err_class:
	class_destroy(bd->cls);
err_cdev:
	cdev_del(&bd->cdev);
err_region:
	unregister_chrdev_region(bd->devno, 1);
err_pwm:
	pwm_put(bd->pwm);
	return ret;
}

static void __exit buzzer_exit(void)
{
	struct buzzer_dev *bd = &g_bdev;
	struct pwm_state   state;

	cancel_delayed_work_sync(&bd->beep_stop_work);

	mutex_lock(&bd->lock);
	pwm_get_state(bd->pwm, &state);
	state.enabled = false;
	pwm_apply_might_sleep(bd->pwm, &state);
	mutex_unlock(&bd->lock);

	device_destroy(bd->cls, bd->devno);
	class_destroy(bd->cls);
	cdev_del(&bd->cdev);
	unregister_chrdev_region(bd->devno, 1);
	pwm_put(bd->pwm);

	pr_info("buzzer: unloaded\n");
}

module_init(buzzer_init);
module_exit(buzzer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("clockOS");
MODULE_DESCRIPTION("Passive Buzzer Character Device Driver – GPIO 13 / PWM1");
MODULE_VERSION("1.0");
