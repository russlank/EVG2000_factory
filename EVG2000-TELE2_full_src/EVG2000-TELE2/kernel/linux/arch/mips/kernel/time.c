/*
 * Copyright 2001 MontaVista Software Inc.
 * Author: Jun Sun, jsun@mvista.com or jsun@junsun.net
 * Copyright (c) 2003, 2004  Maciej W. Rozycki
 *
 * Common time service routines for MIPS machines. See
 * Documentation/mips/time.README.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/param.h>
#include <linux/time.h>
#include <linux/timex.h>
#include <linux/smp.h>
#include <linux/kernel_stat.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/module.h>

#include <asm/bootinfo.h>
#include <asm/cache.h>
#include <asm/compiler.h>
#include <asm/cpu.h>
#include <asm/cpu-features.h>
#include <asm/div64.h>
#include <asm/sections.h>
#include <asm/time.h>

/* Fiji added start pling 08/04/2008 */
#include <linux/netdevice.h>

#define LED_BLINK_RATE_FAST     50
#define LED_BLINK_RATE_NORMAL   20
#define LED_BLINK_RATE_WPS      50
#define LED_BLINK_RATE_VOIP     50

#define WLAN_IF_NAME            "wl0"

#define GPIO_WPS_LED1       4   //RED
#define GPIO_WPS_LED2       5   //GREEN
#define GPIO_LAN_LED        24
#define LED_ON              0
#define LED_OFF             1

#define GPIO_WAN_LED        27 //5
#define WAN_LED_ON          0
#define WAN_LED_OFF         1

//#define GPIO_ANTENA_0_IN_USE    12

/* Fiji added start, Silver Shih 2008/10/21,@USB6368 */
#define GPIO_USB_LED        15
//#define GPIO_USB_1_LED      14  
//#define GPIO_USB_2_LED      2  
#define USB_LED_ON          0
#define USB_LED_OFF         1

int usb_pkt_cnt = 0;
EXPORT_SYMBOL(usb_pkt_cnt);
int usb_led_state = 0;
EXPORT_SYMBOL(usb_led_state);

int usb_1_pkt_cnt = 0;
EXPORT_SYMBOL(usb_1_pkt_cnt);
int usb_2_pkt_cnt = 0;
EXPORT_SYMBOL(usb_2_pkt_cnt);
int usb_1_led_state = 0;
EXPORT_SYMBOL(usb_1_led_state);
int usb_2_led_state = 0;
EXPORT_SYMBOL(usb_2_led_state);

char lan_if_name[4][32];
EXPORT_SYMBOL(lan_if_name);

char wan_if_name[8][32];
EXPORT_SYMBOL(wan_if_name);

typedef void gpioFn(int gpio, int state);
gpioFn *gpioFnPtr = NULL;
static unsigned long gpioFnAddr = 0;

/* Fiji added start, Silver Shih 2008/11/11 */
int lan_led_state = 0;
EXPORT_SYMBOL(lan_led_state);

int wan_led_state = 0;
EXPORT_SYMBOL(wan_led_state);
int wan_pkt_cnt = 0;
EXPORT_SYMBOL(wan_pkt_cnt);

static int pattern = 0;

int voip_led_state = 0;
EXPORT_SYMBOL(voip_led_state);
int voip_led_state_bs = 0;
EXPORT_SYMBOL(voip_led_state_bs);
int voip_led_state_bf = 0;
EXPORT_SYMBOL(voip_led_state_bf);

#define GPIO_VOIP1      14
#define GPIO_VOIP2      2

#define BIT_MASK_VOIP1  0x0004
#define BIT_MASK_VOIP2  0x0008
#define BIT_MASK_VOIP3  0x0010
#define BIT_MASK_VOIP4  0x0020

int wps_led_state = 0;
EXPORT_SYMBOL(wps_led_state);

static void board_led_output(void)
{

    if ( gpioFnPtr == NULL )
        return;

    if ( pattern & BIT_MASK_VOIP1 )
        gpioFnPtr(GPIO_VOIP1, LED_ON);
    else
        gpioFnPtr(GPIO_VOIP1, LED_OFF);

    if ( pattern & BIT_MASK_VOIP2 )
        gpioFnPtr(GPIO_VOIP2, LED_ON);
    else
        gpioFnPtr(GPIO_VOIP2, LED_OFF);    
/*
    if ( pattern & BIT_MASK_VOIP3 )
        gpioFnPtr(GPIO_VOIP3, LED_ON);
    else
        gpioFnPtr(GPIO_VOIP3, LED_OFF);
        
    if ( pattern & BIT_MASK_VOIP4 )
        gpioFnPtr(GPIO_VOIP4, LED_ON);
    else
        gpioFnPtr(GPIO_VOIP4, LED_OFF);
*/
}


static void led_output(int gpio, int state)
{
    if (gpioFnPtr)
        gpioFnPtr(gpio, state);
}

static void wps_err_blink(void)
{
    static int interrupt_count = -1;

    interrupt_count++;
    if (interrupt_count == LED_BLINK_RATE_NORMAL * 2)
        interrupt_count = 0;
    
    if (interrupt_count == 0)
    {
        pattern |= 0x3; 
        board_led_output();
    }
    else
    if (interrupt_count == LED_BLINK_RATE_NORMAL)
    {
        pattern &= ~0x3; 
        board_led_output();
    }
}

static int usb_led_normal_blink(void)
{
    static int interrupt_count = 0;
    static int usb_pkt_cnt_old = 0;
    
    interrupt_count++;
    if (interrupt_count == LED_BLINK_RATE_NORMAL/2)
    {
        if (usb_pkt_cnt != usb_pkt_cnt_old) 
        {
            usb_pkt_cnt_old = usb_pkt_cnt;
            led_output(GPIO_USB_LED, USB_LED_OFF);
        }
    }
    if (interrupt_count < LED_BLINK_RATE_NORMAL)
        return 0;
        
    interrupt_count = 0;
    led_output(GPIO_USB_LED, USB_LED_ON);

    return 0;
}

#if 0
static int usb_1_led_normal_blink(void)
{
    static int interrupt_count = 0;
    static int usb_1_pkt_cnt_old = 0;
    
    interrupt_count++;
    if (interrupt_count == LED_BLINK_RATE_NORMAL/2)
    {
        if (usb_1_pkt_cnt != usb_1_pkt_cnt_old) 
        {
            usb_1_pkt_cnt_old = usb_1_pkt_cnt;
            led_output(GPIO_USB_1_LED, USB_LED_OFF);
        }
    }
    if (interrupt_count < LED_BLINK_RATE_NORMAL)
        return 0;
        
    interrupt_count = 0;
    led_output(GPIO_USB_1_LED, USB_LED_ON);

    return 0;
}

static int usb_2_led_normal_blink(void)
{
    static int interrupt_count = 0;
    static int usb_2_pkt_cnt_old = 0;
    
    interrupt_count++;
    if (interrupt_count == LED_BLINK_RATE_NORMAL/2)
    {
        if (usb_2_pkt_cnt != usb_2_pkt_cnt_old) 
        {
            usb_2_pkt_cnt_old = usb_2_pkt_cnt;
            led_output(GPIO_USB_2_LED, USB_LED_OFF);
        }
    }
    if (interrupt_count < LED_BLINK_RATE_NORMAL)
        return 0;
        
    interrupt_count = 0;
    led_output(GPIO_USB_2_LED, USB_LED_ON);

    return 0;
}
#endif

static void wps_blink(void)
{
    static int interrupt_count = -1;

    interrupt_count++;
    if (interrupt_count == LED_BLINK_RATE_WPS * 2)
        interrupt_count = 0;
    
    if (interrupt_count == 0)
    {
        if (wan_led_state==2)  /* ON-LINE LED , need to check WAN current status */
            led_output(GPIO_WPS_LED2, LED_OFF);
        else
            led_output(GPIO_WPS_LED1, LED_OFF); //red
    }
    else
    if (interrupt_count == LED_BLINK_RATE_WPS)
    {
        if (wan_led_state==2)
            led_output(GPIO_WPS_LED2, LED_ON);
        else
            led_output(GPIO_WPS_LED1, LED_ON); //red
    }
}


static int lan_led_normal_blink(void)
{
    static struct net_device *lan_dev = NULL;
    static struct net_device_stats *lan_1_dev_stats = NULL;
    static struct net_device_stats *lan_2_dev_stats = NULL;
    static struct net_device_stats *lan_3_dev_stats = NULL;
    static struct net_device_stats *lan_4_dev_stats = NULL;

    static int interrupt_count = 0;
    static int pkt_cnt_old = 0;
    int    pkt_cnt = 0;
    
    

    interrupt_count++;
    if (interrupt_count == LED_BLINK_RATE_FAST/2)
        led_output(GPIO_LAN_LED, LED_ON);
    if (interrupt_count < LED_BLINK_RATE_FAST)
        return 0;
 
    interrupt_count = 0;

    if (lan_1_dev_stats && lan_2_dev_stats && lan_3_dev_stats && lan_4_dev_stats)
    {
        pkt_cnt = lan_1_dev_stats->tx_packets + lan_1_dev_stats->rx_packets
                + lan_2_dev_stats->tx_packets + lan_2_dev_stats->rx_packets
                + lan_3_dev_stats->tx_packets + lan_3_dev_stats->rx_packets
                + lan_4_dev_stats->tx_packets + lan_4_dev_stats->rx_packets;
    }
    else 
    {
        //lan_dev = dev_get_by_name(lan_if_name[0]);
        lan_dev = dev_get_by_name("eth1");
        if(lan_dev)
        {
            lan_1_dev_stats = lan_dev->get_stats(lan_dev);
            dev_put(lan_dev);
        }
        //lan_dev = dev_get_by_name(lan_if_name[1]);
        lan_dev = dev_get_by_name("eth2");
        if(lan_dev)
        {
            lan_2_dev_stats = lan_dev->get_stats(lan_dev);
            dev_put(lan_dev);
        }
        //lan_dev = dev_get_by_name(lan_if_name[2]);
        lan_dev = dev_get_by_name("eth3");
        if(lan_dev)
        {
            lan_3_dev_stats = lan_dev->get_stats(lan_dev);
            dev_put(lan_dev);
        }
        //lan_dev = dev_get_by_name(lan_if_name[3]);
        lan_dev = dev_get_by_name("eth4");
        if(lan_dev)
        {
            lan_4_dev_stats = lan_dev->get_stats(lan_dev);
            dev_put(lan_dev);
        }
        
        if (lan_1_dev_stats && lan_2_dev_stats && lan_3_dev_stats && lan_4_dev_stats)
        {
        
            pkt_cnt = lan_1_dev_stats->tx_packets + lan_1_dev_stats->rx_packets
                    + lan_2_dev_stats->tx_packets + lan_2_dev_stats->rx_packets
                    + lan_3_dev_stats->tx_packets + lan_3_dev_stats->rx_packets
                    + lan_4_dev_stats->tx_packets + lan_4_dev_stats->rx_packets;
        }

    }

    if (pkt_cnt != pkt_cnt_old) {
        pkt_cnt_old = pkt_cnt;
        led_output(GPIO_LAN_LED, LED_OFF);
    }

    return 0;
}
/* Fiji added end Bob 11/24/2008 */

static int wan_led_normal_blink(void)
{
    static struct net_device *wan_dev = NULL;
    static struct net_device_stats *wan_1_dev_stats = NULL;

    static int interrupt_count = 0;
    static int pkt_cnt_old = 0;
    int    pkt_cnt = 0;

    interrupt_count++;
    if (interrupt_count == LED_BLINK_RATE_FAST/2)
        led_output(GPIO_WAN_LED, WAN_LED_ON);
    if (interrupt_count < LED_BLINK_RATE_FAST)
        return 0;

    interrupt_count = 0;

    if (wan_1_dev_stats)
    {
        pkt_cnt = wan_1_dev_stats->tx_packets + wan_1_dev_stats->rx_packets;
    }
    else 
    {
        //wan_dev = dev_get_by_name(wan_if_name[0]);
        wan_dev = dev_get_by_name("eth0");
        if(wan_dev)
        {
            wan_1_dev_stats = wan_dev->get_stats(wan_dev);
            dev_put(wan_dev);
        }
        
        if (wan_1_dev_stats)
        {
            pkt_cnt = wan_1_dev_stats->tx_packets + wan_1_dev_stats->rx_packets;
        }

    }

    if (pkt_cnt != pkt_cnt_old) {
        pkt_cnt_old = pkt_cnt;
        led_output(GPIO_WAN_LED, LED_OFF);
    }
    
    return 0;
}
/* Fiji added end Bob 11/24/2008 */

/*
 * The integer part of the number of usecs per jiffy is taken from tick,
 * but the fractional part is not recorded, so we calculate it using the
 * initial value of HZ.  This aids systems where tick isn't really an
 * integer (e.g. for HZ = 128).
 */
#define USECS_PER_JIFFY		TICK_SIZE
#define USECS_PER_JIFFY_FRAC	((unsigned long)(u32)((1000000ULL << 32) / HZ))

#define TICK_SIZE	(tick_nsec / 1000)

/*
 * forward reference
 */
DEFINE_SPINLOCK(rtc_lock);

/*
 * By default we provide the null RTC ops
 */
static unsigned long null_rtc_get_time(void)
{
	return mktime(2000, 1, 1, 0, 0, 0);
}

static int null_rtc_set_time(unsigned long sec)
{
	return 0;
}

unsigned long (*rtc_mips_get_time)(void) = null_rtc_get_time;
int (*rtc_mips_set_time)(unsigned long) = null_rtc_set_time;
int (*rtc_mips_set_mmss)(unsigned long);


/* how many counter cycles in a jiffy */
static unsigned long cycles_per_jiffy __read_mostly;

/* expirelo is the count value for next CPU timer interrupt */
static unsigned int expirelo;


/*
 * Null timer ack for systems not needing one (e.g. i8254).
 */
static void null_timer_ack(void) { /* nothing */ }

/*
 * Null high precision timer functions for systems lacking one.
 */
static cycle_t null_hpt_read(void)
{
	return 0;
}

/*
 * Timer ack for an R4k-compatible timer of a known frequency.
 */
static void c0_timer_ack(void)
{
	unsigned int count;

	/* Ack this timer interrupt and set the next one.  */
	expirelo += cycles_per_jiffy;
	write_c0_compare(expirelo);

	/* Check to see if we have missed any timer interrupts.  */
	while (((count = read_c0_count()) - expirelo) < 0x7fffffff) {
		/* missed_timer_count++; */
		expirelo = count + cycles_per_jiffy;
		write_c0_compare(expirelo);
	}
}

/*
 * High precision timer functions for a R4k-compatible timer.
 */
static cycle_t c0_hpt_read(void)
{
	return read_c0_count();
}

/* For use both as a high precision timer and an interrupt source.  */
static void __init c0_hpt_timer_init(void)
{
	expirelo = read_c0_count() + cycles_per_jiffy;
	write_c0_compare(expirelo);
}

int (*mips_timer_state)(void);
void (*mips_timer_ack)(void);

/* last time when xtime and rtc are sync'ed up */
static long last_rtc_update;

/*
 * local_timer_interrupt() does profiling and process accounting
 * on a per-CPU basis.
 *
 * In UP mode, it is invoked from the (global) timer_interrupt.
 *
 * In SMP mode, it might invoked by per-CPU timer interrupt, or
 * a broadcasted inter-processor interrupt which itself is triggered
 * by the global timer interrupt.
 */
void local_timer_interrupt(int irq, void *dev_id)
{
	profile_tick(CPU_PROFILING);
	update_process_times(user_mode(get_irq_regs()));
}

/*
 * High-level timer interrupt service routines.  This function
 * is set as irqaction->handler and is invoked through do_IRQ.
 */
irqreturn_t timer_interrupt(int irq, void *dev_id)
{
	write_seqlock(&xtime_lock);

	mips_timer_ack();

	/*
	 * call the generic timer interrupt handling
	 */
	do_timer(1);

	/*
	 * If we have an externally synchronized Linux clock, then update
	 * CMOS clock accordingly every ~11 minutes. rtc_mips_set_time() has to be
	 * called as close as possible to 500 ms before the new second starts.
	 */
	if (ntp_synced() &&
	    xtime.tv_sec > last_rtc_update + 660 &&
	    (xtime.tv_nsec / 1000) >= 500000 - ((unsigned) TICK_SIZE) / 2 &&
	    (xtime.tv_nsec / 1000) <= 500000 + ((unsigned) TICK_SIZE) / 2) {
		if (rtc_mips_set_mmss(xtime.tv_sec) == 0) {
			last_rtc_update = xtime.tv_sec;
		} else {
			/* do it again in 60 s */
			last_rtc_update = xtime.tv_sec - 600;
		}
	}

	write_sequnlock(&xtime_lock);

	/*
	 * In UP mode, we call local_timer_interrupt() to do profiling
	 * and process accouting.
	 *
	 * In SMP mode, local_timer_interrupt() is invoked by appropriate
	 * low-level local timer interrupt handler.
	 */
	local_timer_interrupt(irq, dev_id);
	
	/* Fiji added start pling 08/04/2008 */
    /* Blink LED depending of WPS status */
/*
    if (!gpioFnAddr)
    {
        gpioFnAddr = kallsyms_lookup_name("SetGpio");
        if (gpioFnAddr)
        {
            gpioFnPtr = (gpioFn*)gpioFnAddr;
        }
    }
*/    
    if (lan_led_state)
    {
        lan_led_normal_blink();
    }
    else
    {
        led_output(GPIO_LAN_LED, LED_OFF);
    }
    
    if (wan_led_state!=0)
    {
        wan_led_normal_blink();
    }
    else
    {
        led_output(GPIO_WAN_LED, WAN_LED_OFF);
    }

/* Fiji added start, Silver Shih 2008/10/21,@BoardLED */
    static int interrupt_count2 = 0;
     
    interrupt_count2++;
    if ( 0 == (interrupt_count2 % LED_BLINK_RATE_VOIP))
    {
        pattern &= 0x3;
        if (interrupt_count2 == LED_BLINK_RATE_VOIP)
        {
            pattern = pattern | voip_led_state | voip_led_state_bs | voip_led_state_bf; 
            board_led_output();
        
        }
        else if (interrupt_count2 == LED_BLINK_RATE_VOIP*2)
        {
            pattern = pattern | voip_led_state | voip_led_state_bs; 
            board_led_output();
        }
        else if (interrupt_count2 == LED_BLINK_RATE_VOIP*3)
        {
            pattern = pattern | voip_led_state | voip_led_state_bf; 
            board_led_output();
        }
        else if (interrupt_count2 == LED_BLINK_RATE_VOIP*4)
        {
            pattern = pattern | voip_led_state; 
            interrupt_count2=0;
            board_led_output();
        }
    }
/* Fiji added end, Silver Shih 2008/10/21,@BoardLED */
    

    /* WPS use ON-LINE LED so we don't need to care the LED state during WPS is not running */
//    if (!wps_led_state)
//    {
//        led_output(GPIO_ANT1_LED, LED_OFF);
//        led_output(GPIO_WPS_LED2, LED_OFF);
//    }
//    else
    if (wps_led_state == 2)
    {
        wps_blink();
    }

    /* Fiji added start Bob 08/04/2008 */
#if 1    
    if (usb_led_state)
    {
        usb_led_normal_blink();
    }
    else
    {
        led_output(GPIO_USB_LED, USB_LED_OFF);
    }
    /* Fiji added end Bob 08/04/2008 */
#endif
    if (usb_1_led_state)
    {
        //usb_1_led_normal_blink();
    }
    else
    {
        //led_output(GPIO_USB_1_LED, USB_LED_OFF);
    }
    
    if (usb_2_led_state)
    {
        //usb_2_led_normal_blink();
    }
    else
    {
        //led_output(GPIO_USB_2_LED, USB_LED_OFF);
    }
    
	return IRQ_HANDLED;
}

int null_perf_irq(void)
{
	return 0;
}

int (*perf_irq)(void) = null_perf_irq;

EXPORT_SYMBOL(null_perf_irq);
EXPORT_SYMBOL(perf_irq);

/*
 * Performance counter IRQ or -1 if shared with timer
 */
int mipsxx_perfcount_irq;
EXPORT_SYMBOL(mipsxx_perfcount_irq);

/*
 * Possibly handle a performance counter interrupt.
 * Return true if the timer interrupt should not be checked
 */
static inline int handle_perf_irq (int r2)
{
	/*
	 * The performance counter overflow interrupt may be shared with the
	 * timer interrupt (mipsxx_perfcount_irq < 0). If it is and a
	 * performance counter has overflowed (perf_irq() == IRQ_HANDLED)
	 * and we can't reliably determine if a counter interrupt has also
	 * happened (!r2) then don't check for a timer interrupt.
	 */
	return (mipsxx_perfcount_irq < 0) &&
		perf_irq() == IRQ_HANDLED &&
		!r2;
}

asmlinkage void ll_timer_interrupt(int irq)
{
	int r2 = cpu_has_mips_r2;

	irq_enter();
	kstat_this_cpu.irqs[irq]++;

	if (handle_perf_irq(r2))
		goto out;

	if (r2 && ((read_c0_cause() & (1 << 30)) == 0))
		goto out;

	timer_interrupt(irq, NULL);

out:
	irq_exit();
}

asmlinkage void ll_local_timer_interrupt(int irq)
{
	irq_enter();
	if (smp_processor_id() != 0)
		kstat_this_cpu.irqs[irq]++;

	/* we keep interrupt disabled all the time */
	local_timer_interrupt(irq, NULL);

	irq_exit();
}

/*
 * time_init() - it does the following things.
 *
 * 1) board_time_init() -
 * 	a) (optional) set up RTC routines,
 *      b) (optional) calibrate and set the mips_hpt_frequency
 *	    (only needed if you intended to use cpu counter as timer interrupt
 *	     source)
 * 2) setup xtime based on rtc_mips_get_time().
 * 3) calculate a couple of cached variables for later usage
 * 4) plat_timer_setup() -
 *	a) (optional) over-write any choices made above by time_init().
 *	b) machine specific code should setup the timer irqaction.
 *	c) enable the timer interrupt
 */

void (*board_time_init)(void);

unsigned int mips_hpt_frequency;

static struct irqaction timer_irqaction = {
	.handler = timer_interrupt,
	.flags = IRQF_DISABLED | IRQF_PERCPU,
	.name = "timer",
};

static unsigned int __init calibrate_hpt(void)
{
	cycle_t frequency, hpt_start, hpt_end, hpt_count, hz;

	const int loops = HZ / 10;
	int log_2_loops = 0;
	int i;

	/*
	 * We want to calibrate for 0.1s, but to avoid a 64-bit
	 * division we round the number of loops up to the nearest
	 * power of 2.
	 */
	while (loops > 1 << log_2_loops)
		log_2_loops++;
	i = 1 << log_2_loops;

	/*
	 * Wait for a rising edge of the timer interrupt.
	 */
	while (mips_timer_state());
	while (!mips_timer_state());

	/*
	 * Now see how many high precision timer ticks happen
	 * during the calculated number of periods between timer
	 * interrupts.
	 */
	hpt_start = clocksource_mips.read();
	do {
		while (mips_timer_state());
		while (!mips_timer_state());
	} while (--i);
	hpt_end = clocksource_mips.read();

	hpt_count = (hpt_end - hpt_start) & clocksource_mips.mask;
	hz = HZ;
	frequency = hpt_count * hz;

	return frequency >> log_2_loops;
}

struct clocksource clocksource_mips = {
	.name		= "MIPS",
	.mask		= 0xffffffff,
	.flags		= CLOCK_SOURCE_IS_CONTINUOUS,
};

static void __init init_mips_clocksource(void)
{
	u64 temp;
	u32 shift;

	if (!mips_hpt_frequency || clocksource_mips.read == null_hpt_read)
		return;

	/* Calclate a somewhat reasonable rating value */
	clocksource_mips.rating = 200 + mips_hpt_frequency / 10000000;
	/* Find a shift value */
	for (shift = 32; shift > 0; shift--) {
		temp = (u64) NSEC_PER_SEC << shift;
		do_div(temp, mips_hpt_frequency);
		if ((temp >> 32) == 0)
			break;
	}
	clocksource_mips.shift = shift;
	clocksource_mips.mult = (u32)temp;

	clocksource_register(&clocksource_mips);
}

void __init time_init(void)
{
	if (board_time_init)
		board_time_init();

	if (!rtc_mips_set_mmss)
		rtc_mips_set_mmss = rtc_mips_set_time;

	xtime.tv_sec = rtc_mips_get_time();
	xtime.tv_nsec = 0;

	set_normalized_timespec(&wall_to_monotonic,
	                        -xtime.tv_sec, -xtime.tv_nsec);

	/* Choose appropriate high precision timer routines.  */
	if (!cpu_has_counter && !clocksource_mips.read)
		/* No high precision timer -- sorry.  */
		clocksource_mips.read = null_hpt_read;
	else if (!mips_hpt_frequency && !mips_timer_state) {
		/* A high precision timer of unknown frequency.  */
		if (!clocksource_mips.read)
			/* No external high precision timer -- use R4k.  */
			clocksource_mips.read = c0_hpt_read;
	} else {
		/* We know counter frequency.  Or we can get it.  */
		if (!clocksource_mips.read) {
			/* No external high precision timer -- use R4k.  */
			clocksource_mips.read = c0_hpt_read;

			if (!mips_timer_state) {
				/* No external timer interrupt -- use R4k.  */
				mips_timer_ack = c0_timer_ack;
				/* Calculate cache parameters.  */
				cycles_per_jiffy =
					(mips_hpt_frequency + HZ / 2) / HZ;
				/*
				 * This sets up the high precision
				 * timer for the first interrupt.
				 */
				c0_hpt_timer_init();
			}
		}
		if (!mips_hpt_frequency)
			mips_hpt_frequency = calibrate_hpt();

		/* Report the high precision timer rate for a reference.  */
		printk("Using %u.%03u MHz high precision timer.\n",
		       ((mips_hpt_frequency + 500) / 1000) / 1000,
		       ((mips_hpt_frequency + 500) / 1000) % 1000);
	}

	if (!mips_timer_ack)
		/* No timer interrupt ack (e.g. i8254).  */
		mips_timer_ack = null_timer_ack;

	/*
	 * Call board specific timer interrupt setup.
	 *
	 * this pointer must be setup in machine setup routine.
	 *
	 * Even if a machine chooses to use a low-level timer interrupt,
	 * it still needs to setup the timer_irqaction.
	 * In that case, it might be better to set timer_irqaction.handler
	 * to be NULL function so that we are sure the high-level code
	 * is not invoked accidentally.
	 */
	plat_timer_setup(&timer_irqaction);

	init_mips_clocksource();
}

#define FEBRUARY		2
#define STARTOFTIME		1970
#define SECDAY			86400L
#define SECYR			(SECDAY * 365)
#define leapyear(y)		((!((y) % 4) && ((y) % 100)) || !((y) % 400))
#define days_in_year(y)		(leapyear(y) ? 366 : 365)
#define days_in_month(m)	(month_days[(m) - 1])

static int month_days[12] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

void to_tm(unsigned long tim, struct rtc_time *tm)
{
	long hms, day, gday;
	int i;

	gday = day = tim / SECDAY;
	hms = tim % SECDAY;

	/* Hours, minutes, seconds are easy */
	tm->tm_hour = hms / 3600;
	tm->tm_min = (hms % 3600) / 60;
	tm->tm_sec = (hms % 3600) % 60;

	/* Number of years in days */
	for (i = STARTOFTIME; day >= days_in_year(i); i++)
		day -= days_in_year(i);
	tm->tm_year = i;

	/* Number of months in days left */
	if (leapyear(tm->tm_year))
		days_in_month(FEBRUARY) = 29;
	for (i = 1; day >= days_in_month(i); i++)
		day -= days_in_month(i);
	days_in_month(FEBRUARY) = 28;
	tm->tm_mon = i - 1;		/* tm_mon starts from 0 to 11 */

	/* Days are what is left over (+1) from all that. */
	tm->tm_mday = day + 1;

	/*
	 * Determine the day of week
	 */
	tm->tm_wday = (gday + 4) % 7;	/* 1970/1/1 was Thursday */
}

EXPORT_SYMBOL(rtc_lock);
EXPORT_SYMBOL(to_tm);
EXPORT_SYMBOL(rtc_mips_set_time);
EXPORT_SYMBOL(rtc_mips_get_time);
