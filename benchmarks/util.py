import time


def measure_latency(func, *args):
    """
    Measure latency of functions in milliseconds
    """
    start = time.time()
    func(*args)
    end = time.time()
    return (end - start) * 1000