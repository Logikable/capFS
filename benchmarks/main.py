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
(options, args) = parser.parse_args()


if __name__ == '__main__':
    print('options: {}'.format(options))
    print('args: {}'.format(args))

    mode = options.mode
    capfs_dir = options.cap_fs_dir
    efs_dir = options.efs_dir

    if mode == 'micro':
        benchmark = MicroBenchmark(capfs_dir, efs_dir)
        benchmark.run()
    elif mode == 'macro':
        pass
    else:
        print('mode should be either micro or macro')

