import os
import glob

from util import measure_latency


class MicroBenchmark:
    def __init__(self, cap_fs_mount_point, efs_mount_point):
        if cap_fs_mount_point[-1] == '/':
            raise Exception('do not include trailing / to the path for {}'.format(cap_fs_mount_point))
        if efs_mount_point[-1] == '/':
            raise Exception('do not include trailing / to the path for {}'.format(efs_mount_point))

        self.cap_fs_mount_point = cap_fs_mount_point
        self.efs_mount_point = efs_mount_point

    def run(self,
            data_block_size=1024*32,
            iteration_cnt=100,
            read=True,
            write=True,
            write_with_fsync=False,
            create=True,
            create_frequency=3000):
        """
        Run micro-benchmarking
        :param data_block_size(Int): data block size to perform read/write for each iteration_cnt
        :param iteration_cnt(Int): number of times we will read/write blocks with data_block_size
        :param read(Bool): whether or not we will run read micro benchmark
        :param write(Bool): whether or not we will run write micro benchmark
        :param write_with_fsync(Bool): if true, for each iteration_cnt, fsync will be performed
        :param create(Bool): whether or not we will run write micro benchmark
        :param create_frequency(Int): number of files to create for create micro benchmark
        :return:
        """
        if read:
            self.read_benchmark(iteration_cnt, data_block_size)
            self._clean()
            print('\n\n')

        if write:
            self.write_benchmark(iteration_cnt, data_block_size, write_with_fsync)
            self._clean()
            print('\n\n')

        if create:
            self.create_benchmark(create_frequency)
            self._clean()
            print('\n\n')

    def read_benchmark(self, iteration_cnt, block_size):
        print('=' * 60)
        print('Mount point: {}'.format(self.cap_fs_mount_point))
        print('Perform {} number of {} size read'.format(iteration_cnt, block_size))
        capfs_latency = self._read(iteration_cnt, block_size, self.cap_fs_mount_point, 'capfs_read.txt')
        print('read latency for CapFS {} times of {} block size: {}ms'
              .format(iteration_cnt, block_size, capfs_latency))
        print('=' * 60)

    def write_benchmark(self, iteration_cnt, block_size, write_with_fsync):
        print('=' * 60)
        print('Mount point: {}'.format(self.cap_fs_mount_point))
        print('Perform {} number of {} size read'.format(iteration_cnt, block_size))
        print('Perform fsync for each iteration_cnt: {}'.format(write_with_fsync))
        capfs_latency = self._write(iteration_cnt, block_size, self.cap_fs_mount_point, with_fsync=write_with_fsync)
        print('write latency for CapFS {} times of {} block size: {}ms'
              .format(iteration_cnt, block_size, capfs_latency))
        print('=' * 60)

    def create_benchmark(self, frequency):
        print('=' * 60)
        print('Mount point: {}'.format(self.cap_fs_mount_point))
        print('Creating {} number of files at {}'.format(frequency, self.cap_fs_mount_point))
        cap_fs_latency = self._create(frequency, self.cap_fs_mount_point, 'capfs_create.txt')
        print('create latency for CapFS with frequency {}: {} ms'.format(frequency, cap_fs_latency))
        print('=' * 60)

    """
    Private functions
    """
    def _create(self, frequency, mount_point, filename):
        """
        Create frequency number of files on mount_point and return avg latencies
        """
        if not mount_point:
            raise Exception('Provide mount point')

        latencies = 0

        for i in range(frequency):
            latencies += measure_latency(
                self._creat,
                '{mount_point}/{filename}_{number}.txt'.format(mount_point=mount_point, filename=filename, number=i))
        return latencies / frequency

    def _read(self, iteration_cnt, block_size, mount_point, filename):
        """
        Perform iteration_cnt number of block_size read on mount_point and return avg latencies
        """
        filepath = '{}/{}'.format(mount_point, filename)
        file_size = iteration_cnt * block_size
        fd = None
        latencies = 0
        self._creat(filepath)

        try:
            # write bytes before reading
            fd = os.open(filepath, os.O_TRUNC|os.O_RDWR)
            bytes_written = os.write(fd, str.encode('a' * file_size))
            # print('bytes written: {}'.format(bytes_written))
            os.fsync(fd)
            os.lseek(fd, 0, 0)

            for _ in range(iteration_cnt):
                os.read(fd, block_size)
                latencies += measure_latency(os.read, fd, block_size)

        except Exception as e:
            print('problem has occured while read benchmarking')
            print(e.__traceback__)
            print(e)
        finally:
            if fd is None:
                raise Exception('benchmarking failed during some I/O operations')
            else:
                os.close(fd)

        return latencies / iteration_cnt

    def _write(self, iteration_cnt, block_size, mount_point, with_fsync=False):
        """
        Perform iteration_cnt number of block_size write on mount_point and return avg latencies
        """
        filename = 'write_test.txt'
        filepath = '{}/{}'.format(mount_point, filename)
        fd = None
        latencies = 0
        self._creat(filepath)

        try:
            fd = os.open(filepath, os.O_TRUNC|os.O_RDWR)
            for _ in range(iteration_cnt):
                str_to_write = str.encode('a' * block_size)
                latencies += measure_latency(os.write, fd, str_to_write)
                if with_fsync:
                    os.fsync(fd)

        except Exception as e:
            print('problem has occured while write benchmarking')
            print(e.__traceback__)
            print(e)
        finally:
            if fd is None:
                raise Exception('benchmarking failed during some I/O operations')
            else:
                os.close(fd)

        return latencies / iteration_cnt

    def _clean(self):
        capfs_files = glob.glob('{capfs_dir}/*'.format(capfs_dir=self.cap_fs_mount_point))
        efs_files = glob.glob('{efs_dir}/*'.format(efs_dir=self.efs_mount_point))
        for f in capfs_files + efs_files:
            os.remove(f)

    def _creat(self, filename):
        # Create a file with maximum access control
        # Note that this will close the fd right away.
        fd = os.open(filename, os.O_CREAT|os.O_TRUNC|os.O_RDWR, 0o777)
        os.close(fd)
        return fd






