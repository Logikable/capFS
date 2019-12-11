from optparse import OptionParser

from microbenchmark import MicroBenchmark
from macrobenchmark import MacroBenchmark

parser = OptionParser()
parser.add_option('-m', '--mode',
                  action='store', type='string', dest='mode', default='micro',
                  help='Select benchmarking mode. The value should be either micro or macro')
parser.add_option('-c', '--cap-fs-dir',
                  action='store', type='string', dest='cap_fs_dir',
                  help='Directory in which CapFS is mounted. Should be an abstract path')
parser.add_option('-e', '--efs-dir',
                  action='store', type='string', dest='efs_dir',
                  help='Directory in which EFS is mounted. Should be an abstract path')
parser.add_option('-s', '--clean-files',
                  action='store_true', dest='clean_files',
                  help='If set, files created will be cleaned after performing benchmark.'
                       'Note that it can delete all the files in your capFS directory '
                       '(There is a safe mechanism, but be careful just in case.)')
parser.add_option('-i', '--iteration-cnt',
                  action='store', type='int', dest='iteration_cnt', default=100,
                  help='Total number of iteration count to be performed for read/write micro benchmarking.')
parser.add_option('-f', '--create-frequency',
                  action='store', type='int', dest='frequency', default=3000,
                  help='Total number of iteration count to be performed for create micro benchmarking.')
(options, args) = parser.parse_args()


if __name__ == '__main__':
    print('options: {}'.format(options))

    mode = options.mode
    capfs_dir = options.cap_fs_dir
    efs_dir = options.efs_dir
    clean_files = options.clean_files
    iteration_cnt = options.iteration_cnt
    frequency = options.frequency

    if mode == 'micro':
        benchmark = MicroBenchmark(capfs_dir, efs_dir)
        benchmark.run(create_frequency=frequency, iteration_cnt=iteration_cnt, clean_files=clean_files)
    elif mode == 'macro':
        benchmark = MacroBenchmark(capfs_dir, efs_dir)
        benchmark.run(iteration_cnt=iteration_cnt, clean_files=clean_files)
    else:
        print('mode should be either micro or macro')

