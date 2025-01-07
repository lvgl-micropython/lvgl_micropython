# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import machine


class I2C(object):
    class Bus(object):

        def __init__(self, host, scl, sda, freq=400000, use_locks=False):
            self._bus = machine.I2C(
                host,
                scl=machine.Pin(scl),
                sda=machine.Pin(sda),
                freq=freq
            )

            if use_locks:
                import _thread
                self._lock = _thread.allocate_lock()
            else:
                class Lock(object):

                    @staticmethod
                    def acquire():
                        pass

                    @staticmethod
                    def release():
                        pass

                    @staticmethod
                    def is_locked():
                        return False

                self._lock = Lock

        def __enter__(self):
            self._lock.acquire()
            return self

        def __exit__(self, exc_type, exc_val, exc_tb):
            self._lock.release()

        def scan(self):
            self._lock.acquire()
            data = self._bus.scan()
            self._lock.release()
            return data

        def start(self):
            self._bus.start()

        def stop(self):
            self._bus.stop()

        def readinto(self, buf, nack=True):
            self._bus.readinto(buf, nack)

        def write(self, buf):
            self._bus.write(buf)

        def readfrom(self, addr, nbytes, stop=True):
            return self._bus.readfrom(addr, nbytes, stop)

        def readfrom_into(self, addr, buf, stop=True):
            self._bus.readfrom_into(addr, buf, stop)

        def writeto(self, addr, buf, stop=True):
            self._bus.writeto(addr, buf, stop)

        def writevto(self, addr, vector, stop=True):
            self._bus.writevto(addr, vector, stop)

        def readfrom_mem(self, addr, memaddr, nbytes, addrsize=8):
            return self._bus.readfrom_mem(addr, memaddr,
                                          nbytes, addrsize=addrsize)

        def readfrom_mem_into(self, addr, memaddr, buf, addrsize=8):
            self._bus.readfrom_mem_into(addr, memaddr, buf, addrsize=addrsize)

        def writeto_mem(self, addr, memaddr, buf, addrsize=8):
            self._bus.writeto_mem(addr, memaddr, buf, addrsize=addrsize)

    class Device(object):

        def __init__(self, bus, dev_id, reg_bits=8):
            self._bus = bus
            self.dev_id = dev_id
            self._reg_bits = reg_bits

        def write_readinto(self, write_buf, read_buf):
            memaddr = 0

            for i in range(int(self._reg_bits / 8) - 1, -1, -1):
                memaddr |= write_buf[i] << ((~i + int(self._reg_bits / 8)) * 8)

            self.read_mem(memaddr, buf=read_buf)

        def read_mem(self, memaddr, num_bytes=None, buf=None):
            with self._bus:
                if num_bytes is not None:
                    return self._bus.readfrom_mem(
                        self.dev_id,
                        memaddr,
                        num_bytes,
                        addrsize=self._reg_bits
                    )
                else:
                    self._bus.readfrom_mem_into(
                        self.dev_id,
                        memaddr,
                        buf,
                        addrsize=self._reg_bits
                    )
                    return

        def write_mem(self, memaddr, buf):
            with self._bus:
                self._bus.writeto_mem(
                    self.dev_id,
                    memaddr,
                    buf,
                    addrsize=self._reg_bits
                )

        def read(self, num_bytes=None, buf=None):
            with self._bus:
                if num_bytes is not None:
                    return self._bus.readfrom(self.dev_id, num_bytes)
                else:
                    self._bus.readfrom_into(self.dev_id, buf)

        def write(self, buf):
            self._bus.writeto(self.dev_id, buf)
