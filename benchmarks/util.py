import time
from enum import Enum


class Unit(Enum):
    KB = 1024
    MB = 1024 * 1024
    B = 1
    GB = 1024 * 1024 * 1024


def translation_unit(unit):
    if unit == Unit.B:
        return 'B'
    elif unit == Unit.KB:
        return 'KB'
    elif unit == Unit.MB:
        return 'MB'
    elif unit == Unit.GB:
        return 'GB'
    else:
        raise Exception('Wrong unit is given. It should be B, KB, MB, or GB')


def measure_latency(func, *args):
    """
    Measure latency of functions in milliseconds
    """
    start = time.time()
    func(*args)
    end = time.time()
    return (end - start) * 1000


def measure_latency_with_return_val(func, *args):
    """
    Measure latency of functions in milliseconds
    """
    start = time.time()
    ret = func(*args)
    end = time.time()
    return (end - start) * 1000, ret


def get_throughput(latency, block_size, unit=Unit.KB):
    """
    Measure throughput based on latency. Note that latency input should be in milliseconds.
    NOTE: It will return string
    """
    latency_in_s = latency / 1000
    throughput = (1 / latency_in_s) * block_size
    return '{} {}'.format(round(throughput / unit.value, ndigits=3), translation_unit(unit))

