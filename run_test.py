import subprocess
import os

test_dir = os.getcwd() + '/src/test'
test_files = os.listdir(test_dir)
print(test_files)

for test_file in test_files:
    if test_file.endswith('c'):
        print(test_file)
        subprocess.run(['make', 'test', 'TEST={test_file_name}'.format(test_file_name=test_file)])
