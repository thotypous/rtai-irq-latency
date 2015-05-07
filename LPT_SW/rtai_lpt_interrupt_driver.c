#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>

#define KB  1024
#define MB  1024*KB

#define FIFO_DT      0

// Parallel port setup
#define LPT      0x378
#define LPT_STS  LPT+1
#define LPT_CTRL LPT+2
#define LPT_IRQ  5

// Periodic task period in ns
#define PERIOD 100000

RTIME timestamp_before_write;

RT_TASK thread;
int period_counts;

// -------- Interrupt handler --------
static int irq_handler(unsigned irq, void *cookie_) {
    const RTIME diff = rt_get_time_ns() - timestamp_before_write;
    const uint8_t data = 0xFF; //inb(LPT);
    const int irq_is_ours = (data & 0x1) == 1;

    if (likely(irq_is_ours)) {
        rtf_put(FIFO_DT, (void*)&diff, sizeof(diff));
        outb(0x00, LPT);
    }

    rt_unmask_irq(irq);
    return irq_is_ours;
}


// -------- Periodic realtime thread --------
void thread_main(long thread) {
    while (1) {
        rt_task_wait_period();
        timestamp_before_write = rt_get_time_ns();
        outb(0xFF, LPT);
    }
}

/*******************************
 *  Driver Init/Exit Functions *
 *******************************/

static int lpt_interrupt_init(void) {
    int retval;

    rtf_create(FIFO_DT, 16*MB);

    rt_task_init(&thread, thread_main, 0, 5000, 0, 0, 0);
    rt_set_runnable_on_cpus(&thread, 3);

    rt_set_oneshot_mode();
    period_counts = start_rt_timer(nano2count(PERIOD));

    // Enable IRQ Via Ack Line
    outb(0x10, LPT_CTRL);

    rt_printk("lpt_interrupt_driver: registering for IRQ=%d\n", LPT_IRQ);

    // Request IRQ and install handler
    retval = rt_request_irq(LPT_IRQ, irq_handler, NULL, 0);
    if(retval) {
        rt_printk("lpt_interrupt_driver: request_irq failed!\n");
        return retval;
    }
        
    rt_startup_irq(LPT_IRQ);

    // Start at zero to cause a IRQ on posedge
    outb(0x00, LPT);

    // Start the task
    {
        const RTIME expected = rt_get_time() + 10 * period_counts;
        rt_task_make_periodic(&thread, expected, period_counts);
    }

    return retval;
}

static void lpt_interrupt_exit(void) {
    rt_release_irq(LPT_IRQ);

    rt_printk("lpt_interrupt_driver: stopping timer and task.\n");
    stop_rt_timer();
    rt_task_delete(&thread);

    rtf_destroy(FIFO_DT);
    
    rt_printk("lpt_interrupt_driver: lpt driver unregistered.\n");
}

module_init(lpt_interrupt_init);
module_exit(lpt_interrupt_exit);
MODULE_LICENSE("GPL");
