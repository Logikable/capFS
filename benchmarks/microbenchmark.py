import os, time
import glob


class MicroBenchmark:
    def __init__(self, cap_fs_mount_point, efs_mount_point):
        self.cap_fs_mount_point = cap_fs_mount_point
        self.efs_mount_point = efs_mount_point

    def run(self,
            create_frequency=3000,
            read_benchmark=True,
            write_benchmark=True,
            create_benchmark=True):
        if read_benchmark:
            self._read_benchmark()
            self._clean()

        if write_benchmark:
            self._write_benchmark()
            self._clean()

        if create_benchmark:
            create_latency = self._measure_latency(self._create_benchmark, create_frequency)
            print('create_latency: {}ms'.format(create_latency))
            self._clean()

    def _read_benchmark(self):
        pass

    def _write_benchmark(self):
        pass

    def _create_benchmark(self, frequency):
        for i in range(frequency):
            fd = self._creat('{capfs_dir}/random_{number}.txt'
                             .format(capfs_dir=self.cap_fs_mount_point, number=i))

    def _measure_latency(self, func, *args):
        """
        Measure latency of functions in milliseconds
        """
        start = time.time()
        func(*args)
        end = time.time()
        return (end - start) * 1000

    def _clean(self):
        capfs_files = glob.glob('{capfs_dir}/*'.format(capfs_dir=self.cap_fs_mount_point))
        efs_files = glob.glob('{efs_dir}/*'.format(efs_dir=self.efs_mount_point))
        for f in capfs_files + efs_files:
            os.remove(f)

    def _creat(self, filename):
        # Create a file with maximum access control
        fd = os.open(filename, os.O_CREAT|os.O_TRUNC|os.O_RDWR, 0o777)
        os.close(fd)
        return fd





