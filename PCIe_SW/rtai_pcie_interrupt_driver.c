#include <linux/module.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>

#define KB  1024
#define MB  1024*KB

#define FIFO_DT      0

// Defines addresses configured on QSys (bar0)
#define PCIE_ECHOMOD 0x0000

// Defines addresses configured on QSys (bar2)
#define PCIE_CRA     0x0000

// Defines some CRA addresses
#define CRA_INTSTAREG  0x40

// Defines for the EchoModule Interface
#define ECHOMOD_SEND   0x00
#define ECHOMOD_RECV   0x04

// Periodic task period in ns
#define PERIOD 100000

static void *echomod_base;

static uint8_t n_devices = 0;

static uint32_t epoch = 1;

RTIME timestamp_before_write;

RT_TASK thread;
int period_counts;

// -------- Interrupt handler --------
static int irq_handler(unsigned irq, void *cookie_) {
    const RTIME diff = rt_get_time_ns() - timestamp_before_write;
    const uint32_t flag = readl(echomod_base + ECHOMOD_RECV);
    const int irq_is_ours = (flag == epoch);

    if (likely(irq_is_ours)) {
        rtf_put(FIFO_DT, (void*)&diff, sizeof(diff));
        ++epoch;
    }

    rt_unmask_irq(irq);
    return irq_is_ours;
}

// -------- Periodic realtime thread --------
void thread_main(long thread) {
    while (1) {
        rt_task_wait_period();
        timestamp_before_write = rt_get_time_ns();
        writel(epoch, echomod_base + ECHOMOD_SEND);
    }
}


static struct pci_device_id pci_ids[] = {
    { PCI_DEVICE(0x1172, 0x0de4), }, // Demo numbers
    { 0, }
};
MODULE_DEVICE_TABLE(pci, pci_ids);

static int pci_probe(struct pci_dev *dev, const struct pci_device_id *id);
static void pci_remove(struct pci_dev *dev);

static struct pci_driver pci_driver = {
    .name       = "pcie_interrupt",
    .id_table   = pci_ids,
    .probe      = pci_probe,
    .remove     = pci_remove,
};

static int pci_probe(struct pci_dev *dev, const struct pci_device_id *id) {
    unsigned long resource;
    int retval;

    if(++n_devices > 1)
        return -EOVERFLOW;

    retval = pci_enable_device(dev);

    if(retval) {
        rt_printk("pcie_interrupt_driver: ERROR: Unable to enable device: %d\n",retval);
        return retval;
    }
    else
        rt_printk("pcie_interrupt_driver: device enabled!\n");

    // Gets a pointer to bar0
    resource = pci_resource_start(dev,0);
    echomod_base = ioremap_nocache(resource + PCIE_ECHOMOD, 16);

    // Read vendor ID
    rt_printk("pcie_interrupt_driver: Found Vendor id: 0x%0.4x\n", dev->vendor);
    rt_printk("pcie_interrupt_driver: Found Device id: 0x%0.4x\n", dev->device);

    // Read IRQ Number
    rt_printk("pcie_interrupt_driver: Found IRQ: %d\n", dev->irq);

    // Request IRQ and install handler
    retval = rt_request_irq(dev->irq, irq_handler, NULL, 0);
    if(retval) {
        rt_printk("pcie_interrupt_driver: request_irq failed!\n");
        return retval;
    }
        
    rt_startup_irq(dev->irq);

    // Start the task
    {
        const RTIME expected = rt_get_time() + 10 * period_counts;
        rt_task_make_periodic(&thread, expected, period_counts);
    }

    return 0;
}

static void pci_remove(struct pci_dev *dev) {
    
    rt_release_irq(dev->irq);

    --n_devices;

    iounmap(echomod_base);

    pci_set_drvdata(dev,NULL);
    rt_printk("pcie_interrupt_driver: Interrupt handler uninstalled.\n");
}

/*******************************
 *  Driver Init/Exit Functions *
 *******************************/

static int pcie_interrupt_init(void) {
    int retval;

    rtf_create(FIFO_DT, 16*MB);

    rt_task_init(&thread, thread_main, 0, 5000, 0, 0, 0);
    rt_set_runnable_on_cpus(&thread, 3);

    rt_set_oneshot_mode();
    period_counts = start_rt_timer(nano2count(PERIOD));

    // Register PCI Driver
    // IRQ is requested on pci_probe
    retval = pci_register_driver(&pci_driver);
    if (retval)
        rt_printk("pcie_interrupt_driver: ERROR: cannot register pci.\n");
    else
        rt_printk("pcie_interrupt_driver: pci driver registered.\n");

    return retval;
}

static void pcie_interrupt_exit(void) {
    rt_printk("pcie_interrupt_driver: stopping timer and task.\n");
    stop_rt_timer();
    rt_task_delete(&thread);

    rtf_destroy(FIFO_DT);
    
    pci_unregister_driver(&pci_driver);
    rt_printk("pcie_interrupt_driver: pci driver unregistered.\n");
}

module_init(pcie_interrupt_init);
module_exit(pcie_interrupt_exit);
MODULE_LICENSE("GPL");
