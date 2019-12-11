import time


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