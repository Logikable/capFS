"""
Note:
    You can use util.py's measure_latency to check latency
    For each benchmarking, utilize subprocess library at Python.
    For sequential read/write, you can probably just reuse microbenchmark code.

    If you want to use bash script instead, feel free to do so!

    Look at microbenchmark._read_benchmark code.
"""
from microbenchmark import MicroBenchmark


class MacroBenchmark:
    def __init__(self, cap_fs_mount_point, efs_mount_point):
        if cap_fs_mount_point[-1] == '/':
            raise Exception('do not include trailing / to the path for {}'.format(cap_fs_mount_point))
        if efs_mount_point[-1] == '/':
            raise Exception('do not include trailing / to the path for {}'.format(efs_mount_point))

        self.cap_fs_mount_point = cap_fs_mount_point
        self.efs_mount_point = efs_mount_point
        self.microbenchmark = MicroBenchmark(cap_fs_mount_point, efs_mount_point)

    def run(self):
        self.run_sequential_read()
        self.run_sequential_write()
        self.run_random_read()
        self.run_random_write()
        self.run_cp()
        self.run_untarring()

    def run_sequential_read(self):
        pass

    def run_sequential_write(self):
        pass

    def run_random_read(self):
        pass

    def run_random_write(self):
        pass

    def run_cp(self):
        pass

    def run_untarring(self):
        pass