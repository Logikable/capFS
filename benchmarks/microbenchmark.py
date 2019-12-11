import os
import glob

from util import measure_latency, measure_latency_with_return_val


class MicroBenchmark:
    def __init__(self, cap_fs_mount_point, efs_mount_point):
        if cap_fs_mount_point[-1] == '/':
            raise Exception('do not include trailing / to the path for {}'.format(cap_fs_mount_point))
        if efs_mount_point[-1] == '/':
            raise Exception('do not include trailing / to the path for {}'.format(efs_mount_point))

        # created files' path while performing benchmarking
        self.file_path_created = []
        self.cap_fs_mount_point = cap_fs_mount_point
        self.efs_mount_point = efs_mount_point

    def run(self,
            data_block_size=1024*32,
            iteration_cnt=100,
            read=True,
            write=True,
            write_with_fsync=False,
            create=True,
            create_frequency=3000,
            clean_files=True,
            open=True):
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
            if clean_files:
                self._clean()
            print('\n\n')

        if write:
            self.write_benchmark(iteration_cnt, data_block_size, write_with_fsync)
            if clean_files:
                self._clean()
            print('\n\n')

        if create:
            self.create_benchmark(create_frequency)
            if clean_files:
                self._clean()
            print('\n\n')

        if open:
            self.open_benchmark(iteration_cnt)
            self._clean()
            print('\n\n')

    def read_benchmark(self, iteration_cnt, block_size):
        self._read_benchmark(iteration_cnt,
                             block_size,
                             self.cap_fs_mount_point,
                             'capfs_read.txt',
                             'capfs')
        self._read_benchmark(iteration_cnt,
                             block_size,
                             self.efs_mount_point,
                             'efs_read.txt',
                             'efs')

    def write_benchmark(self, iteration_cnt, block_size, write_with_fsync):
        self._write_benchmark(iteration_cnt,
                              block_size,
                              write_with_fsync,
                              self.cap_fs_mount_point,
                              'capfs_write.txt',
                              'capfs')
        self._write_benchmark(iteration_cnt,
                              block_size,
                              write_with_fsync,
                              self.efs_mount_point,
                              'efs_write.txt',
                              'efs')

    def create_benchmark(self, frequency):
        self._create_benchmark(frequency,
                               self.cap_fs_mount_point,
                               'capfs_create.txt',
                               'capfs')
        self._create_benchmark(frequency,
                               self.efs_mount_point,
                               'efs_create.txt',
                               'efs')

    def open_benchmark(self, iteration_cnt):
        self._open_benchmark(iteration_cnt,
                             self.cap_fs_mount_point,
                             'capfs_open.txt',
                             'capfs')
        self._open_benchmark(iteration_cnt,
                             self.efs_mount_point,
                             'efs_open.txt',
                             'efs')
    """
    Private functions
    """
    def _read_benchmark(self, iteration_cnt, block_size, mount_point, read_file_name, filesys_name):
        print('=' * 60)
        print('READ micro benchmark')
        print('Mount point: {}'.format(mount_point))
        print('Perform {} number of {} size read'.format(iteration_cnt, block_size))
        latency = self._read(iteration_cnt, block_size, mount_point, read_file_name)
        print('filesystem: {}. read latency for {} times of {} block size: {}ms'
              .format(filesys_name, iteration_cnt, block_size, latency))
        print('=' * 60)

    def _write_benchmark(self, iteration_cnt, block_size, write_with_fsync, mount_point, write_file_name, filesys_name):
        print('=' * 60)
        print('WRITE micro benchmark')
        print('Mount point: {}'.format(mount_point))
        print('Perform {} number of {} size read'.format(iteration_cnt, block_size))
        print('Perform fsync for each iteration_cnt: {}'.format(write_with_fsync))
        latency = self._write(iteration_cnt, block_size, mount_point, write_file_name, with_fsync=write_with_fsync)
        print('filesys: {}. write latency {} times of {} block size: {}ms'
              .format(filesys_name, iteration_cnt, block_size, latency))
        print('=' * 60)

    def _create_benchmark(self, frequency, mount_point, create_file_name, filesys_name):
        print('=' * 60)
        print('CREATE micro benchmark')
        print('Mount point: {}'.format(self.cap_fs_mount_point))
        print('Creating {} number of files at {}'.format(frequency, mount_point))
        latency = self._create(frequency, mount_point, create_file_name)
        print('filesys: {}. create latency with frequency {}: {} ms'.format(filesys_name, frequency, latency))
        print('=' * 60)

    def _open_benchmark(self, iteration_cnt, mount_point, filename, filesys_name):
        print('=' * 60)
        print('OPEN micro benchmark')
        print('Mount point: {}'.format(self.cap_fs_mount_point))
        print('Open {} file {} number of times'.format(filename, iteration_cnt))
        latency = self._open(iteration_cnt, mount_point, filename)
        print('filesys: {}. avg open latency with iteration_cnt {}: {} ms'
              .format(filesys_name, iteration_cnt, latency))
        print('=' * 60)

    def _create(self, frequency, mount_point, filename):
        """
        Create frequency number of files on mount_point and return avg latencies
        """
        if not mount_point:
            raise Exception('Provide mount point')

        latencies = 0

        for i in range(frequency):
            file_path = '{mount_point}/{filename}_{number}.txt'\
                .format(mount_point=mount_point, filename=filename, number=i)
            self.file_path_created.append(file_path)
            latencies += measure_latency(self._creat, file_path)
        return latencies / frequency

    def _read(self, iteration_cnt, block_size, mount_point, filename):
        """
        Perform iteration_cnt number of block_size read on mount_point and return avg latencies
        """
        filepath = '{}/{}'.format(mount_point, filename)
        self.file_path_created.append(filepath)
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

    def _write(self, iteration_cnt, block_size, mount_point, filename, with_fsync=False):
        """
        Perform iteration_cnt number of block_size write on mount_point and return avg latencies
        """
        filepath = '{}/{}'.format(mount_point, filename)
        self.file_path_created.append(filepath)
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

    def _open(self, iteration_cnt, mount_point, filename):
        filepath = '{}/{}'.format(mount_point, filename)
        self.file_path_created.append(filepath)
        fd = None
        latencies = 0
        self._creat(filepath)

        try:
            for _ in range(iteration_cnt):
                latency, fd = measure_latency_with_return_val(os.open, filepath, os.O_TRUNC|os.O_RDWR)
                latencies += latency
                os.close(fd)
                fd = None

        except Exception as e:
            print('problem has occured while write benchmarking')
            print(e.__traceback__)
            print(e)
        finally:
            if fd is not None:
                os.close(fd)

        return latencies / iteration_cnt

    def _clean(self):
        for filepath in self.file_path_created:
            os.remove(filepath)
        self.file_path_created = []

    def _creat(self, filename):
        # Create a file with maximum access control
        # Note that this will close the fd right away.
        fd = os.open(filename, os.O_CREAT|os.O_TRUNC|os.O_RDWR, 0o777)
        os.close(fd)
        return fd

