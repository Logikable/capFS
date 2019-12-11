"""
Note:
    You can use util.py's measure_latency to check latency
    For each benchmarking, utilize subprocess library at Python.
    For sequential read/write, you can probably just reuse microbenchmark code.

    If you want to use bash script instead, feel free to do so!

    Look at microbenchmark._read_benchmark code.
"""
import tarfile, glob, random, os, subprocess, shutil
from microbenchmark import MicroBenchmark
from util import measure_latency

class MacroBenchmark:
    def __init__(self, cap_fs_mount_point, efs_mount_point):
        self.file_path_created = []
        if cap_fs_mount_point[-1] == '/':
            raise Exception('do not include trailing / to the path for {}'.format(cap_fs_mount_point))
        if efs_mount_point[-1] == '/':
            raise Exception('do not include trailing / to the path for {}'.format(efs_mount_point))

        self.cap_fs_mount_point = cap_fs_mount_point
        self.efs_mount_point = efs_mount_point
        self.microbenchmark = MicroBenchmark(cap_fs_mount_point, efs_mount_point)

    def run(self,
            data_block_size=1024*32,
            iteration_cnt=100,
            untar_iteration_cnt=3,
            sequential_read=True,
            sequential_write=True,
            random_read=True,
            random_write=True,
            write_with_fsync=False,
            cp=True,
            untarring=True,
            clean_files=True):
        if sequential_read:
            self.microbenchmark.read_benchmark(iteration_cnt, data_block_size)
            if clean_files:
                self._clean()
            print('\n\n')

        if sequential_write:
            self.microbenchmark.write_benchmark(iteration_cnt, data_block_size, write_with_fsync)
            if clean_files:
                self._clean()
            print('\n\n')

        if random_read:
            self.random_read_benchmark(iteration_cnt, data_block_size)
            if clean_files:
                self._clean()
            print('\n\n')

        if random_write:
            self.random_write_benchmark(iteration_cnt, data_block_size, write_with_fsync)
            if clean_files:
                self._clean()
            print('\n\n')

        if cp:
            self.cp_benchmark(iteration_cnt, data_block_size)
            if clean_files:
                self._clean()
            print('\n\n')

        if untarring:
            self.untar_benchmark(untar_iteration_cnt, data_block_size)
            if clean_files:
                self._clean()
            print('\n\n')

    def random_read_benchmark(self, iteration_cnt, block_size):
        self._random_read_benchmark(iteration_cnt,
                                     block_size,
                                     self.cap_fs_mount_point,
                                     'capfs_read.txt',
                                     'capfs')
        self._random_read_benchmark(iteration_cnt,
                                     block_size,
                                     self.efs_mount_point,
                                     'efs_read.txt',
                                     'efs')

    def random_write_benchmark(self, iteration_cnt, block_size, write_with_fsync):
        self._random_write_benchmark(iteration_cnt,
                                      block_size,
                                      write_with_fsync,
                                      self.cap_fs_mount_point,
                                      'capfs_write.txt',
                                      'capfs')
        self._random_write_benchmark(iteration_cnt,
                                      block_size,
                                      write_with_fsync,
                                      self.efs_mount_point,
                                      'efs_write.txt',
                                      'efs')

    def cp_benchmark(self, iteration_cnt, block_size):
        self._cp_benchmark(iteration_cnt,
                            block_size,
                            self.cap_fs_mount_point,
                            'capfs_read.txt',
                            'capfs')
        self._cp_benchmark(iteration_cnt,
                            block_size,
                            self.efs_mount_point,
                            'efs_read.txt',
                            'efs')

    def untar_benchmark(self, iteration_cnt, block_size):
        self._untar_benchmark(iteration_cnt,
                            block_size,
                            self.cap_fs_mount_point,
                            'capFS.tar.gz',
                            'capfs')
        self._untar_benchmark(iteration_cnt,
                            block_size,
                            self.efs_mount_point,
                            'capFS.tar.gz',
                            'efs')

    def _random_read_benchmark(self, iteration_cnt, block_size, mount_point, read_file_name, filesys_name):
        print('=' * 60)
        print('RANDOM READ macro benchmark')
        print('Mount point: {}'.format(mount_point))
        print('Perform {} number of {} size read'.format(iteration_cnt, block_size))
        latency = self._random_read(iteration_cnt, block_size, mount_point, read_file_name)
        print('filesystem: {}. RANDOM read latency for {} times of {} block size: {}ms'
              .format(filesys_name, iteration_cnt, block_size, latency))
        print('=' * 60)

    def _random_write_benchmark(self, iteration_cnt, block_size, write_with_fsync, mount_point, write_file_name, filesys_name):
        print('=' * 60)
        print('RANDOM WRITE macro benchmark')
        print('Mount point: {}'.format(mount_point))
        print('Perform {} number of {} size written'.format(iteration_cnt, block_size))
        print('Perform fsync for each iteration_cnt: {}'.format(write_with_fsync))
        latency = self._random_write(iteration_cnt, block_size, mount_point, write_file_name, with_fsync=write_with_fsync)
        print('filesys: {}. RANDOM write latency {} times of {} block size: {}ms'
              .format(filesys_name, iteration_cnt, block_size, latency))
        print('=' * 60)

    def _cp_benchmark(self, iteration_cnt, block_size, mount_point, read_file_name, filesys_name):
        print('=' * 60)
        print('CP macro benchmark')
        print('Mount point: {}'.format(mount_point))
        print('Perform {} number of {} size copying'.format(iteration_cnt, block_size))
        latency = self._run_cp(iteration_cnt, block_size, mount_point, read_file_name)
        print('filesystem: {}. copy latency for {} times of {} block size: {}ms'
              .format(filesys_name, iteration_cnt, block_size, latency))
        print('=' * 60)

    def _untar_benchmark(self, iteration_cnt, block_size, mount_point, read_file_name, filesys_name):
        print('=' * 60)
        print('UNTAR macro benchmark')
        print('Mount point: {}'.format(mount_point))
        print('Perform {} number of {} size untarring'.format(iteration_cnt, block_size))
        latency = self._run_untar(iteration_cnt, block_size, mount_point, read_file_name)
        print('filesystem: {}. untar latency for {} times of {} block size: {}ms'
              .format(filesys_name, iteration_cnt, block_size, latency))
        print('=' * 60)


    def _random_read(self, iteration_cnt, block_size, mount_point, filename):
        """
        Perform iteration_cnt number of block_size random read on mount_point and return avg latencies
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

            for _ in range(iteration_cnt):
                os.lseek(fd, random.randint(0, file_size), 0)
                #os.read(fd, block_size)
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


    def _random_write(self, iteration_cnt, block_size, mount_point, filename, with_fsync=False):
        """
        Perform iteration_cnt number of block_size random write on mount_point and return avg latencies
        """
        filepath = '{}/{}'.format(mount_point, filename)
        self.file_path_created.append(filepath)
        file_size = iteration_cnt * block_size
        fd = None
        latencies = 0
        self._creat(filepath)

        try:
            fd = os.open(filepath, os.O_TRUNC|os.O_RDWR)
            for _ in range(iteration_cnt):
                os.lseek(fd, random.randint(0, file_size), 0)
                str_to_write = str.encode('a' * block_size)
                latencies += measure_latency(os.write, fd, str_to_write)
                if with_fsync:
                    os.fsync(fd)

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

    def _run_cp(self, iteration_cnt, block_size, mount_point, filename):
        src_filepath = '{}/{}'.format(mount_point, filename)
        self.file_path_created.append(src_filepath)
        #create a new dir for /temp and append filepath+name
        dsn_file_dir = '{}/{}'.format(mount_point, "tmp")


        try:
            if not os.path.exists(dsn_file_dir):
                os.mkdir(dsn_file_dir)
        except OSError:
            print ("Creation of the directory %s failed" % dsn_file_path)
        dsn_filepath = '{}/{}'.format(dsn_file_dir, filename)

        latencies = 0
        self._creat(src_filepath)

        try:
            for _ in range(iteration_cnt):
                latencies += measure_latency(subprocess.call, ['cp', '-r', src_filepath, dsn_filepath])

            self.file_path_created.append(dsn_filepath)
            self.file_path_created.append(dsn_file_dir)
        except Exception as e:
            print('problem has occured while write benchmarking')
            print(e.__traceback__)
            print(e)


        return latencies / iteration_cnt

    def _run_untar(self, iteration_cnt, block_size, mount_point, filename):
        filepath = '{}/{}'.format(mount_point, filename)
        latencies = 0
        dsn_file_dir = '{}/{}'.format(mount_point, "untarred")

        try:
            if not os.path.exists(dsn_file_dir):
                os.mkdir(dsn_file_dir)
        except OSError:
            print ("Creation of the directory %s failed" % dsn_file_dir)
        # dsn_file_dir = '{}/{}'.format(mount_point, dsn_file_dir)

        try:
            for _ in range(iteration_cnt):
                latencies += measure_latency(subprocess.call, ['tar', 'xf', filepath, '-C', dsn_file_dir])
            self.file_path_created.append(dsn_file_dir)
        except Exception as e:
            print('problem has occured while write benchmarking')
            print(e.__traceback__)
            print(e)
        return latencies / iteration_cnt


    def _clean(self):
        for filepath in self.file_path_created:
            if os.path.isfile(filepath):
                os.remove(filepath)
            else:
                shutil.rmtree(filepath)
        self.file_path_created = []


    def _creat(self, filename):
        # Create a file with maximum access control
        # Note that this will close the fd right away.
        fd = os.open(filename, os.O_CREAT|os.O_TRUNC|os.O_RDWR, 0o777)
        os.close(fd)
        return fd

