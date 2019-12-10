from optparse import OptionParser

from microbenchmark import MicroBenchmark

parser = OptionParser()
parser.add_option('-m', '--mode',
                  action='store', type='string', dest='mode', default='micro',
                  help='select benchmarking mode. The value should be either micro or macro')
parser.add_option('-c', '--cap-fs-dir',
                  action='store', type='string', dest='cap_fs_dir',
                  help='Directory in which CapFS is mounted. Should be an abstract path')
parser.add_option('-e', '--efs-dir',
                  action='store', type='string', dest='efs_dir',
                  help='Directory in which EFS is mounted. Should be an abstract path')
parser.add_option('-f', '--clean-files',
                  action='store_true', dest='clean_files',
                  help='If set, files created will be cleaned after performing benchmark.'
                       'Note that it can delete all the files in your capFS directory '
                       '(There is a safe mechanism, but be careful just in case.)')
(options, args) = parser.parse_args()


if __name__ == '__main__':
    print('options: {}'.format(options))

    mode = options.mode
    capfs_dir = options.cap_fs_dir
    efs_dir = options.efs_dir
    clean_files = options.clean_files

    if mode == 'micro':
        benchmark = MicroBenchmark(capfs_dir, efs_dir)
        benchmark.run(clean_files=clean_files)
    elif mode == 'macro':
        pass
    else:
        print('mode should be either micro or macro')

