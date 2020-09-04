/*
 * QTest
 *
 * Copyright IBM, Corp. 2012
 * Copyright Red Hat, Inc. 2012
 * Copyright SUSE LINUX Products GmbH 2013
 *
 * Authors:
 *  Anthony Liguori   <aliguori@us.ibm.com>
 *  Paolo Bonzini     <pbonzini@redhat.com>
 *  Andreas Färber    <afaerber@suse.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */
#ifndef LIBQTEST_H
#define LIBQTEST_H

#include "qapi/qmp/qobject.h"
#include "qapi/qmp/qdict.h"

typedef struct QTestState QTestState;

/**
 * qtest_initf:
 * @fmt...: Format for creating other arguments to pass to QEMU, formatted
 * like sprintf().
 *
 * Convenience wrapper around qtest_init().
 *
 * Returns: #QTestState instance.
 */
QTestState *qtest_initf(const char *fmt, ...) GCC_FMT_ATTR(1, 2);

/**
 * qtest_vinitf:
 * @fmt: Format for creating other arguments to pass to QEMU, formatted
 * like vsprintf().
 * @ap: Format arguments.
 *
 * Convenience wrapper around qtest_init().
 *
 * Returns: #QTestState instance.
 */
QTestState *qtest_vinitf(const char *fmt, va_list ap) GCC_FMT_ATTR(1, 0);

typedef int gpio_id;

/* When running a QTest program stand-alone, the following environment variables
 * may be set:
 *
 * Mandatory:
 *  QTEST_QEMU_BINARY: The QEMU binary to use.  It is set automatically when
 *                     running "make check".
 *                     (e.g. "../arm-softmmu/qemu-system-arm")
 *
 * Optional:
 *  QTEST_QEMU_ARGS:   Extra arguments to pass to QEMU (e.g. "-d").
 *  QTEST_LOG_FILE:    A file to which QTest log messages should be written
 *                     (e.g. "/tmp/qtest.log").
 */

/**
 * qtest_init:
 * @extra_args: other arguments to pass to QEMU.  CAUTION: these
 * arguments are subject to word splitting and shell evaluation.
 *
 * Returns: #QTestState instance.
 */
QTestState *qtest_init(const char *extra_args,  int num_serial_ports);

/**
 * qtest_init_without_qmp_handshake:
 * @extra_args: other arguments to pass to QEMU.  CAUTION: these
 * arguments are subject to word splitting and shell evaluation.
 *
 * Returns: #QTestState instance.
 */
QTestState *qtest_init_without_qmp_handshake(const char *extra_args);

/**
 * qtest_init_with_serial:
 * @extra_args: other arguments to pass to QEMU.  CAUTION: these
 * arguments are subject to word splitting and shell evaluation.
 * @sock_fd: pointer to store the socket file descriptor for
 * connection with serial.
 *
 * Returns: #QTestState instance.
 */
QTestState *qtest_init_with_serial(const char *extra_args, int *sock_fd);

/**
 * qtest_quit:
 * @s: #QTestState instance to operate on.
 *
 * Shut down the QEMU process associated to @s.
 */
void qtest_quit(QTestState *s);

/**
 * qtest_qmp_fds:
 * @s: #QTestState instance to operate on.
 * @fds: array of file descriptors
 * @fds_num: number of elements in @fds
 * @fmt...: QMP message to send to qemu, formatted like
 * qobject_from_jsonf_nofail().  See parse_escape() for what's
 * supported after '%'.
 *
 * Sends a QMP message to QEMU with fds and returns the response.
 */
QDict *qtest_qmp_fds(QTestState *s, int *fds, size_t fds_num,
                     const char *fmt, ...)
    GCC_FMT_ATTR(4, 5);

/**
 * qtest_qmp:
 * @s: #QTestState instance to operate on.
 * @fmt...: QMP message to send to qemu, formatted like
 * qobject_from_jsonf_nofail().  See parse_escape() for what's
 * supported after '%'.
 *
 * Sends a QMP message to QEMU and returns the response.
 */
QDict *qtest_qmp(QTestState *s, const char *fmt, ...)
    GCC_FMT_ATTR(2, 3);

/**
 * qtest_qmp_send:
 * @s: #QTestState instance to operate on.
 * @fmt...: QMP message to send to qemu, formatted like
 * qobject_from_jsonf_nofail().  See parse_escape() for what's
 * supported after '%'.
 *
 * Sends a QMP message to QEMU and leaves the response in the stream.
 */
void qtest_qmp_send(QTestState *s, const char *fmt, ...)
    GCC_FMT_ATTR(2, 3);

/**
 * qtest_qmp_send_raw:
 * @s: #QTestState instance to operate on.
 * @fmt...: text to send, formatted like sprintf()
 *
 * Sends text to the QMP monitor verbatim.  Need not be valid JSON;
 * this is useful for negative tests.
 */
void qtest_qmp_send_raw(QTestState *s, const char *fmt, ...)
    GCC_FMT_ATTR(2, 3);

/**
 * qtest_vqmp_fds:
 * @s: #QTestState instance to operate on.
 * @fds: array of file descriptors
 * @fds_num: number of elements in @fds
 * @fmt: QMP message to send to QEMU, formatted like
 * qobject_from_jsonf_nofail().  See parse_escape() for what's
 * supported after '%'.
 * @ap: QMP message arguments
 *
 * Sends a QMP message to QEMU with fds and returns the response.
 */
QDict *qtest_vqmp_fds(QTestState *s, int *fds, size_t fds_num,
                      const char *fmt, va_list ap)
    GCC_FMT_ATTR(4, 0);

/**
 * qtest_vqmp:
 * @s: #QTestState instance to operate on.
 * @fmt: QMP message to send to QEMU, formatted like
 * qobject_from_jsonf_nofail().  See parse_escape() for what's
 * supported after '%'.
 * @ap: QMP message arguments
 *
 * Sends a QMP message to QEMU and returns the response.
 */
QDict *qtest_vqmp(QTestState *s, const char *fmt, va_list ap)
    GCC_FMT_ATTR(2, 0);

/**
 * qtest_qmp_vsend_fds:
 * @s: #QTestState instance to operate on.
 * @fds: array of file descriptors
 * @fds_num: number of elements in @fds
 * @fmt: QMP message to send to QEMU, formatted like
 * qobject_from_jsonf_nofail().  See parse_escape() for what's
 * supported after '%'.
 * @ap: QMP message arguments
 *
 * Sends a QMP message to QEMU and leaves the response in the stream.
 */
void qtest_qmp_vsend_fds(QTestState *s, int *fds, size_t fds_num,
                         const char *fmt, va_list ap)
    GCC_FMT_ATTR(4, 0);

/**
 * qtest_qmp_vsend:
 * @s: #QTestState instance to operate on.
 * @fmt: QMP message to send to QEMU, formatted like
 * qobject_from_jsonf_nofail().  See parse_escape() for what's
 * supported after '%'.
 * @ap: QMP message arguments
 *
 * Sends a QMP message to QEMU and leaves the response in the stream.
 */
void qtest_qmp_vsend(QTestState *s, const char *fmt, va_list ap)
    GCC_FMT_ATTR(2, 0);

/**
 * qtest_receive:
 * @s: #QTestState instance to operate on.
 *
 * Reads a QMP message from QEMU and returns the response.
 */
QDict *qtest_qmp_receive(QTestState *s);

/**
 * qtest_qmp_eventwait:
 * @s: #QTestState instance to operate on.
 * @s: #event event to wait for.
 *
 * Continuously polls for QMP responses until it receives the desired event.
 */
void qtest_qmp_eventwait(QTestState *s, const char *event);

/**
 * qtest_qmp_eventwait_ref:
 * @s: #QTestState instance to operate on.
 * @s: #event event to wait for.
 *
 * Continuously polls for QMP responses until it receives the desired event.
 * Returns a copy of the event for further investigation.
 */
QDict *qtest_qmp_eventwait_ref(QTestState *s, const char *event);

/**
 * qtest_qmp_receive_success:
 * @s: #QTestState instance to operate on
 * @event_cb: Event callback
 * @opaque: Argument for @event_cb
 *
 * Poll QMP messages until a command success response is received.
 * If @event_cb, call it for each event received, passing @opaque,
 * the event's name and data.
 * Return the success response's "return" member.
 */
QDict *qtest_qmp_receive_success(QTestState *s,
                                 void (*event_cb)(void *opaque,
                                                  const char *name,
                                                  QDict *data),
                                 void *opaque);

/**
 * qtest_hmp:
 * @s: #QTestState instance to operate on.
 * @fmt...: HMP command to send to QEMU, formats arguments like sprintf().
 *
 * Send HMP command to QEMU via QMP's human-monitor-command.
 * QMP events are discarded.
 *
 * Returns: the command's output.  The caller should g_free() it.
 */
char *qtest_hmp(QTestState *s, const char *fmt, ...) GCC_FMT_ATTR(2, 3);

/**
 * qtest_hmpv:
 * @s: #QTestState instance to operate on.
 * @fmt: HMP command to send to QEMU, formats arguments like vsprintf().
 * @ap: HMP command arguments
 *
 * Send HMP command to QEMU via QMP's human-monitor-command.
 * QMP events are discarded.
 *
 * Returns: the command's output.  The caller should g_free() it.
 */
char *qtest_vhmp(QTestState *s, const char *fmt, va_list ap)
    GCC_FMT_ATTR(2, 0);

void qtest_module_load(QTestState *s, const char *prefix, const char *libname);

/**
 * qtest_get_irq:
 * @s: #QTestState instance to operate on.
 * @num: Interrupt to observe.
 *
 * Returns: The level of the @num interrupt.
 */
bool qtest_get_irq(QTestState *s, int num);

/**
 * qtest_get_irq_for_gpio:
 * @s: #QTestState instance to operate on.
 * @id: GPIO to operate on (see qtest_irq_intercept_in/out).
 * @num: Interrupt to observe.
 *
 * Returns: The level of the @num interrupt.
 */
bool qtest_get_irq_for_gpio(QTestState *s, gpio_id id, int num);

/**
 * qtest_irq_intercept_in:
 * @s: #QTestState instance to operate on.
 * @string: QOM path of a device.
 *
 * Associate qtest irqs with the GPIO-in pins of the device
 * whose path is specified by @string.
 * Returns an id value that can be used when calling qtest_get_irq.
 */
gpio_id qtest_irq_intercept_in(QTestState *s, const char *string);

/**
 * qtest_irq_intercept_out:
 * @s: #QTestState instance to operate on.
 * @string: QOM path of a device.
 *
 * Associate qtest irqs with the GPIO-out pins of the device
 * whose path is specified by @string.
 * Returns an id value that can be used when calling qtest_get_irq.
 */
gpio_id qtest_irq_intercept_out(QTestState *s, const char *string);

/**
 * qtest_set_irq_in:
 * @s: #QTestState instance to operate on.
 * @string: QOM path of a device.
 * @num: Interrupt to set (specifies a GPIO-in pin)
 * @level: new IRQ level
 *
 * Sets the GPIO-in pin level of the device whose path is specified by @string.
 */
void qtest_set_irq_in(QTestState *s, const char *string, int num, int level);

/**
 * write_serial_port:
 * @s: #QTestState instance to operate on.
 * @serial_port_num: Indicates the serial port to write to
 *                   (see qtest_start_with_serial).
 * @fmt: The printf style format string to write.
 *
 * Sends a string to the specified virtual serial port.
 */
void qtest_write_serial_port(QTestState *s,
                             int serial_port_num,
                             const char *fmt, ...);

/**
 * read_serial_port_byte:
 * @s: #QTestState instance to operate on.
 * @serial_port_num: Indicates the serial port to read from
 *                   (see qtest_start_with_serial).
 *
 * Reads an byte (8-bit value) from the specified virtual serial port.
 * If there is no byte to read, an assertion failure will occur.
 *
 * Returns: Value read.
 */
uint8_t qtest_read_serial_port_byte(QTestState *s, int serial_port_num);

/**
 * qtest_set_irq_in:
 * @s: QTestState instance to operate on.
 * @string: QOM path of a device
 * @name: IRQ name
 * @irq: IRQ number
 * @level: IRQ level
 *
 * Force given device/irq GPIO-in pin to the given level.
 */
void qtest_set_irq_in(QTestState *s, const char *string, const char *name,
                      int irq, int level);

/**
 * qtest_outb:
 * @s: #QTestState instance to operate on.
 * @addr: I/O port to write to.
 * @value: Value being written.
 *
 * Write an 8-bit value to an I/O port.
 */
void qtest_outb(QTestState *s, uint16_t addr, uint8_t value);

/**
 * qtest_outw:
 * @s: #QTestState instance to operate on.
 * @addr: I/O port to write to.
 * @value: Value being written.
 *
 * Write a 16-bit value to an I/O port.
 */
void qtest_outw(QTestState *s, uint16_t addr, uint16_t value);

/**
 * qtest_outl:
 * @s: #QTestState instance to operate on.
 * @addr: I/O port to write to.
 * @value: Value being written.
 *
 * Write a 32-bit value to an I/O port.
 */
void qtest_outl(QTestState *s, uint16_t addr, uint32_t value);

/**
 * qtest_inb:
 * @s: #QTestState instance to operate on.
 * @addr: I/O port to read from.
 *
 * Returns an 8-bit value from an I/O port.
 */
uint8_t qtest_inb(QTestState *s, uint16_t addr);

/**
 * qtest_inw:
 * @s: #QTestState instance to operate on.
 * @addr: I/O port to read from.
 *
 * Returns a 16-bit value from an I/O port.
 */
uint16_t qtest_inw(QTestState *s, uint16_t addr);

/**
 * qtest_inl:
 * @s: #QTestState instance to operate on.
 * @addr: I/O port to read from.
 *
 * Returns a 32-bit value from an I/O port.
 */
uint32_t qtest_inl(QTestState *s, uint16_t addr);

/**
 * qtest_writeb:
 * @s: #QTestState instance to operate on.
 * @addr: Guest address to write to.
 * @value: Value being written.
 *
 * Writes an 8-bit value to memory.
 */
void qtest_writeb(QTestState *s, uint64_t addr, uint8_t value);

/**
 * qtest_writew:
 * @s: #QTestState instance to operate on.
 * @addr: Guest address to write to.
 * @value: Value being written.
 *
 * Writes a 16-bit value to memory.
 */
void qtest_writew(QTestState *s, uint64_t addr, uint16_t value);

/**
 * qtest_writel:
 * @s: #QTestState instance to operate on.
 * @addr: Guest address to write to.
 * @value: Value being written.
 *
 * Writes a 32-bit value to memory.
 */
void qtest_writel(QTestState *s, uint64_t addr, uint32_t value);

/**
 * qtest_writeq:
 * @s: #QTestState instance to operate on.
 * @addr: Guest address to write to.
 * @value: Value being written.
 *
 * Writes a 64-bit value to memory.
 */
void qtest_writeq(QTestState *s, uint64_t addr, uint64_t value);

/**
 * qtest_readb:
 * @s: #QTestState instance to operate on.
 * @addr: Guest address to read from.
 *
 * Reads an 8-bit value from memory.
 *
 * Returns: Value read.
 */
uint8_t qtest_readb(QTestState *s, uint64_t addr);

/**
 * qtest_readw:
 * @s: #QTestState instance to operate on.
 * @addr: Guest address to read from.
 *
 * Reads a 16-bit value from memory.
 *
 * Returns: Value read.
 */
uint16_t qtest_readw(QTestState *s, uint64_t addr);

/**
 * qtest_readl:
 * @s: #QTestState instance to operate on.
 * @addr: Guest address to read from.
 *
 * Reads a 32-bit value from memory.
 *
 * Returns: Value read.
 */
uint32_t qtest_readl(QTestState *s, uint64_t addr);

/**
 * qtest_readq:
 * @s: #QTestState instance to operate on.
 * @addr: Guest address to read from.
 *
 * Reads a 64-bit value from memory.
 *
 * Returns: Value read.
 */
uint64_t qtest_readq(QTestState *s, uint64_t addr);

/**
 * qtest_memread:
 * @s: #QTestState instance to operate on.
 * @addr: Guest address to read from.
 * @data: Pointer to where memory contents will be stored.
 * @size: Number of bytes to read.
 *
 * Read guest memory into a buffer.
 */
void qtest_memread(QTestState *s, uint64_t addr, void *data, size_t size);

/**
 * qtest_rtas_call:
 * @s: #QTestState instance to operate on.
 * @name: name of the command to call.
 * @nargs: Number of args.
 * @args: Guest address to read args from.
 * @nret: Number of return value.
 * @ret: Guest address to write return values to.
 *
 * Call an RTAS function
 */
uint64_t qtest_rtas_call(QTestState *s, const char *name,
                         uint32_t nargs, uint64_t args,
                         uint32_t nret, uint64_t ret);

/**
 * qtest_bufread:
 * @s: #QTestState instance to operate on.
 * @addr: Guest address to read from.
 * @data: Pointer to where memory contents will be stored.
 * @size: Number of bytes to read.
 *
 * Read guest memory into a buffer and receive using a base64 encoding.
 */
void qtest_bufread(QTestState *s, uint64_t addr, void *data, size_t size);

/**
 * qtest_memwrite:
 * @s: #QTestState instance to operate on.
 * @addr: Guest address to write to.
 * @data: Pointer to the bytes that will be written to guest memory.
 * @size: Number of bytes to write.
 *
 * Write a buffer to guest memory.
 */
void qtest_memwrite(QTestState *s, uint64_t addr, const void *data, size_t size);

/**
 * qtest_bufwrite:
 * @s: #QTestState instance to operate on.
 * @addr: Guest address to write to.
 * @data: Pointer to the bytes that will be written to guest memory.
 * @size: Number of bytes to write.
 *
 * Write a buffer to guest memory and transmit using a base64 encoding.
 */
void qtest_bufwrite(QTestState *s, uint64_t addr,
                    const void *data, size_t size);

/**
 * qtest_memset:
 * @s: #QTestState instance to operate on.
 * @addr: Guest address to write to.
 * @patt: Byte pattern to fill the guest memory region with.
 * @size: Number of bytes to write.
 *
 * Write a pattern to guest memory.
 */
void qtest_memset(QTestState *s, uint64_t addr, uint8_t patt, size_t size);

/**
 * qtest_clock_step_next:
 * @s: #QTestState instance to operate on.
 *
 * Advance the QEMU_CLOCK_VIRTUAL to the next deadline.
 *
 * Returns: The current value of the QEMU_CLOCK_VIRTUAL in nanoseconds.
 */
int64_t qtest_clock_step_next(QTestState *s);

/**
 * qtest_clock_step:
 * @s: QTestState instance to operate on.
 * @step: Number of nanoseconds to advance the clock by.
 *
 * Advance the QEMU_CLOCK_VIRTUAL by @step nanoseconds.
 *
 * Returns: The current value of the QEMU_CLOCK_VIRTUAL in nanoseconds.
 */
int64_t qtest_clock_step(QTestState *s, int64_t step);

/**
 * qtest_clock_set:
 * @s: QTestState instance to operate on.
 * @val: Nanoseconds value to advance the clock to.
 *
 * Advance the QEMU_CLOCK_VIRTUAL to @val nanoseconds since the VM was launched.
 *
 * Returns: The current value of the QEMU_CLOCK_VIRTUAL in nanoseconds.
 */
int64_t qtest_clock_set(QTestState *s, int64_t val);

/**
 * qtest_big_endian:
 * @s: QTestState instance to operate on.
 *
 * Returns: True if the architecture under test has a big endian configuration.
 */
bool qtest_big_endian(QTestState *s);

/**
 * qtest_get_arch:
 *
 * Returns: The architecture for the QEMU executable under test.
 */
const char *qtest_get_arch(void);

/**
 * qtest_add_func:
 * @str: Test case path.
 * @fn: Test case function
 *
 * Add a GTester testcase with the given name and function.
 * The path is prefixed with the architecture under test, as
 * returned by qtest_get_arch().
 */
void qtest_add_func(const char *str, void (*fn)(void));

/**
 * qtest_add_data_func:
 * @str: Test case path.
 * @data: Test case data
 * @fn: Test case function
 *
 * Add a GTester testcase with the given name, data and function.
 * The path is prefixed with the architecture under test, as
 * returned by qtest_get_arch().
 */
void qtest_add_data_func(const char *str, const void *data,
                         void (*fn)(const void *));

/**
 * qtest_add_data_func_full:
 * @str: Test case path.
 * @data: Test case data
 * @fn: Test case function
 * @data_free_func: GDestroyNotify for data
 *
 * Add a GTester testcase with the given name, data and function.
 * The path is prefixed with the architecture under test, as
 * returned by qtest_get_arch().
 *
 * @data is passed to @data_free_func() on test completion.
 */
void qtest_add_data_func_full(const char *str, void *data,
                              void (*fn)(const void *),
                              GDestroyNotify data_free_func);

/**
 * qtest_add:
 * @testpath: Test case path
 * @Fixture: Fixture type
 * @tdata: Test case data
 * @fsetup: Test case setup function
 * @ftest: Test case function
 * @fteardown: Test case teardown function
 *
 * Add a GTester testcase with the given name, data and functions.
 * The path is prefixed with the architecture under test, as
 * returned by qtest_get_arch().
 */
#define qtest_add(testpath, Fixture, tdata, fsetup, ftest, fteardown) \
    do { \
        char *path = g_strdup_printf("/%s/%s", qtest_get_arch(), testpath); \
        g_test_add(path, Fixture, tdata, fsetup, ftest, fteardown); \
        g_free(path); \
    } while (0)

void qtest_add_abrt_handler(GHookFunc fn, const void *data);

/**
<<<<<<< HEAD:tests/qtest/libqtest.h
 * qtest_qmp_assert_success:
 * @qts: QTestState instance to operate on
 * @fmt...: QMP message to send to qemu, formatted like
 * qobject_from_jsonf_nofail().  See parse_escape() for what's
 * supported after '%'.
=======
 * qtest_start:
 * @args: other arguments to pass to QEMU
 *
 * Start QEMU and assign the resulting #QTestState to a global variable.
 * The global variable is used by "shortcut" functions documented below.
 *
 * Returns: #QTestState instance.
 */
static inline QTestState *qtest_start(const char *args)
{
    global_qtest = qtest_init(args, 0);
    return global_qtest;
}

/**
 * qtest_start_with_serial:
 * @args: See qtest_start
 * @num_sockets: number of extra sockets to
 *
 * Works the same way as qtest_start, except creates a connection to the
 * specified number of virtual serial ports.
 *
 * Use write_serial_port and read_serial_port_byte to communicate via these
 * connections.  These functions require a serial port number.  The serial
 * ports are numbered in the same order that they are specified on
 * the QEMU command line (using the -serial option).  The first serial port
 * specified on the command line is numbered 0, the second port is numbered 1,
 * and so on.
 */
static inline QTestState *qtest_start_with_serial(const char *args,
                                                  int num_serial_ports)
{
    global_qtest = qtest_init(args, num_serial_ports);
    return global_qtest;
}

/**
 * qtest_end:
 *
 * Shut down the QEMU process started by qtest_start().
 */
static inline void qtest_end(void)
{
    qtest_quit(global_qtest);
    global_qtest = NULL;
}

/**
 * qmp:
 * @fmt...: QMP message to send to qemu
 *
 * Sends a QMP message to QEMU and returns the response.
 */
QDict *qmp(const char *fmt, ...);

/**
 * qmp_async:
 * @fmt...: QMP message to send to qemu
 *
 * Sends a QMP message to QEMU and leaves the response in the stream.
 */
void qmp_async(const char *fmt, ...);

/**
 * qmp_discard_response:
 * @fmt...: QMP message to send to qemu
 *
 * Sends a QMP message to QEMU and consumes the response.
 */
void qmp_discard_response(const char *fmt, ...);

/**
 * qmp_receive:
 *
 * Reads a QMP message from QEMU and returns the response.
 */
static inline QDict *qmp_receive(void)
{
    return qtest_qmp_receive(global_qtest);
}

/**
 * qmp_eventwait:
 * @s: #event event to wait for.
 *
 * Continuosly polls for QMP responses until it receives the desired event.
 */
static inline void qmp_eventwait(const char *event)
{
    return qtest_qmp_eventwait(global_qtest, event);
}

/**
 * hmp:
 * @fmt...: HMP command to send to QEMU
 *
 * Send HMP command to QEMU via QMP's human-monitor-command.
 *
 * Returns: the command's output.  The caller should g_free() it.
 */
char *hmp(const char *fmt, ...);

/**
 * get_irq:
 * @num: Interrupt to observe.
 *
 * Returns: The level of the @num interrupt.
 */
static inline bool get_irq(int num)
{
    return qtest_get_irq(global_qtest, num);
}

/**
 * get_irq_for_gpio:
 * @id: GPIO to operate on (see qtest_irq_intercept_in/out).
 * @num: Interrupt to observe.
 *
 * Returns: The level of the @num interrupt.
 */
static inline bool get_irq_for_gpio(gpio_id id, int num)
{
    return qtest_get_irq_for_gpio(global_qtest, id, num);
}

/**
 * irq_intercept_in:
 * @string: QOM path of a device.
 *
 * Associate qtest irqs with the GPIO-in pins of the device
 * whose path is specified by @string.
 */
static inline void irq_intercept_in(const char *string)
{
    qtest_irq_intercept_in(global_qtest, string);
}

/**
 * qtest_irq_intercept_out:
 * @string: QOM path of a device.
 *
 * Associate qtest irqs with the GPIO-out pins of the device
 * whose path is specified by @string.
 */
static inline void irq_intercept_out(const char *string)
{
    qtest_irq_intercept_out(global_qtest, string);
}

/**
 * set_irq_in:
 * @string: QOM path of a device.
 * @num: Interrupt to set (specifies a GPIO-in pin)
 * @level: new IRQ level
 *
 * Sets the GPIO-in pin level of the device whose path is specified by @string.
 */
static inline void set_irq_in(const char *string, int num, int level)
{
    qtest_set_irq_in(global_qtest, string, num, level);
}

/**
 * write_serial_port:
 * @serial_port_num: Indicates the serial port to write to
 *                   (see qtest_start_with_serial).
 * @fmt: The printf style format string to write.
 *
 * Sends a string to the specified virtual serial port.
 */
void write_serial_port(int serial_port_num, const char *fmt, ...);

/**
 * read_serial_port_byte:
 * @serial_port_num: Indicates the serial port to read from
 *                   (see qtest_start_with_serial).
 *
 * Reads an byte (8-bit value) from the specified virtual serial port.
 * If there is no byte to read, an assertion failure will occur.
 *
 * Returns: Value read.
 */
static inline uint8_t read_serial_port_byte(int serial_port_num)
{
    return qtest_read_serial_port_byte(global_qtest, serial_port_num);
}

/**
 * outb:
 * @addr: I/O port to write to.
 * @value: Value being written.
 *
 * Write an 8-bit value to an I/O port.
 */
static inline void outb(uint16_t addr, uint8_t value)
{
    qtest_outb(global_qtest, addr, value);
}

/**
 * outw:
 * @addr: I/O port to write to.
 * @value: Value being written.
 *
 * Write a 16-bit value to an I/O port.
 */
static inline void outw(uint16_t addr, uint16_t value)
{
    qtest_outw(global_qtest, addr, value);
}

/**
 * outl:
 * @addr: I/O port to write to.
 * @value: Value being written.
 *
 * Write a 32-bit value to an I/O port.
 */
static inline void outl(uint16_t addr, uint32_t value)
{
    qtest_outl(global_qtest, addr, value);
}

/**
 * inb:
 * @addr: I/O port to read from.
 *
 * Reads an 8-bit value from an I/O port.
 *
 * Returns: Value read.
 */
static inline uint8_t inb(uint16_t addr)
{
    return qtest_inb(global_qtest, addr);
}

/**
 * inw:
 * @addr: I/O port to read from.
 *
 * Reads a 16-bit value from an I/O port.
 *
 * Returns: Value read.
 */
static inline uint16_t inw(uint16_t addr)
{
    return qtest_inw(global_qtest, addr);
}

/**
 * inl:
 * @addr: I/O port to read from.
 *
 * Reads a 32-bit value from an I/O port.
 *
 * Returns: Value read.
 */
static inline uint32_t inl(uint16_t addr)
{
    return qtest_inl(global_qtest, addr);
}

/**
 * writeb:
 * @addr: Guest address to write to.
 * @value: Value being written.
 *
 * Writes an 8-bit value to guest memory.
 */
static inline void writeb(uint64_t addr, uint8_t value)
{
    qtest_writeb(global_qtest, addr, value);
}

/**
 * writew:
 * @addr: Guest address to write to.
 * @value: Value being written.
 *
 * Writes a 16-bit value to guest memory.
 */
static inline void writew(uint64_t addr, uint16_t value)
{
    qtest_writew(global_qtest, addr, value);
}

/**
 * writel:
 * @addr: Guest address to write to.
 * @value: Value being written.
 *
 * Writes a 32-bit value to guest memory.
 */
static inline void writel(uint64_t addr, uint32_t value)
{
    qtest_writel(global_qtest, addr, value);
}

/**
 * writeq:
 * @addr: Guest address to write to.
 * @value: Value being written.
 *
 * Writes a 64-bit value to guest memory.
 */
static inline void writeq(uint64_t addr, uint64_t value)
{
    qtest_writeq(global_qtest, addr, value);
}

/**
 * readb:
 * @addr: Guest address to read from.
 *
 * Reads an 8-bit value from guest memory.
>>>>>>> 919b29ba7d... Pebble Qemu:tests/libqtest.h
 *
 * Sends a QMP message to QEMU and asserts that a 'return' key is present in
 * the response.
 */
void qtest_qmp_assert_success(QTestState *qts, const char *fmt, ...)
    GCC_FMT_ATTR(2, 3);

QDict *qmp_fd_receive(int fd);
void qmp_fd_vsend_fds(int fd, int *fds, size_t fds_num,
                      const char *fmt, va_list ap) GCC_FMT_ATTR(4, 0);
void qmp_fd_vsend(int fd, const char *fmt, va_list ap) GCC_FMT_ATTR(2, 0);
void qmp_fd_send(int fd, const char *fmt, ...) GCC_FMT_ATTR(2, 3);
void qmp_fd_send_raw(int fd, const char *fmt, ...) GCC_FMT_ATTR(2, 3);
void qmp_fd_vsend_raw(int fd, const char *fmt, va_list ap) GCC_FMT_ATTR(2, 0);
QDict *qmp_fdv(int fd, const char *fmt, va_list ap) GCC_FMT_ATTR(2, 0);
QDict *qmp_fd(int fd, const char *fmt, ...) GCC_FMT_ATTR(2, 3);

/**
 * qtest_cb_for_every_machine:
 * @cb: Pointer to the callback function
 * @skip_old_versioned: true if versioned old machine types should be skipped
 *
 *  Call a callback function for every name of all available machines.
 */
void qtest_cb_for_every_machine(void (*cb)(const char *machine),
                                bool skip_old_versioned);

/**
 * qtest_qmp_device_add_qdict:
 * @qts: QTestState instance to operate on
 * @drv: Name of the device that should be added
 * @arguments: QDict with properties for the device to intialize
 *
 * Generic hot-plugging test via the device_add QMP command with properties
 * supplied in form of QDict. Use NULL for empty properties list.
 */
void qtest_qmp_device_add_qdict(QTestState *qts, const char *drv,
                                const QDict *arguments);

/**
 * qtest_qmp_device_add:
 * @qts: QTestState instance to operate on
 * @driver: Name of the device that should be added
 * @id: Identification string
 * @fmt...: QMP message to send to qemu, formatted like
 * qobject_from_jsonf_nofail().  See parse_escape() for what's
 * supported after '%'.
 *
 * Generic hot-plugging test via the device_add QMP command.
 */
void qtest_qmp_device_add(QTestState *qts, const char *driver, const char *id,
                          const char *fmt, ...) GCC_FMT_ATTR(4, 5);

/**
 * qtest_qmp_device_del:
 * @qts: QTestState instance to operate on
 * @id: Identification string
 *
 * Generic hot-unplugging test via the device_del QMP command.
 */
void qtest_qmp_device_del(QTestState *qts, const char *id);

/**
 * qmp_rsp_is_err:
 * @rsp: QMP response to check for error
 *
 * Test @rsp for error and discard @rsp.
 * Returns 'true' if there is error in @rsp and 'false' otherwise.
 */
bool qmp_rsp_is_err(QDict *rsp);

/**
 * qmp_assert_error_class:
 * @rsp: QMP response to check for error
 * @class: an error class
 *
 * Assert the response has the given error class and discard @rsp.
 */
void qmp_assert_error_class(QDict *rsp, const char *class);

/**
 * qtest_probe_child:
 * @s: QTestState instance to operate on.
 *
 * Returns: true if the child is still alive.
 */
bool qtest_probe_child(QTestState *s);

/**
 * qtest_set_expected_status:
 * @s: QTestState instance to operate on.
 * @status: an expected exit status.
 *
 * Set expected exit status of the child.
 */
void qtest_set_expected_status(QTestState *s, int status);

QTestState *qtest_inproc_init(QTestState **s, bool log, const char* arch,
                    void (*send)(void*, const char*));

void qtest_client_inproc_recv(void *opaque, const char *str);
#endif
