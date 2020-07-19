import argparse
import time

DEVICE_FILE = '/dev/pid_tracker'


parser = argparse.ArgumentParser(description='TID/PID PCB tracker')
parser.add_argument('--pid', default=-1, type=int, help='process ID')
parser.add_argument('--tid', default=-1, type=int, help='thread ID')
parser.add_argument('--period', default=1, type=int,
                    help='interval period in seconds')
args = parser.parse_args()

pid = args.pid if args.tid == -1 else args.tid
with open(DEVICE_FILE, 'w') as f:
    f.write(str(pid))

while True:
    with open(DEVICE_FILE, 'r', errors='ignore') as f:
        fields = f.readline().split('\t')
        if len(fields) < 5:
            print(
                "Could not find {} - {}".format('process' if args.tid == -1 else 'thread', pid))
            exit(0)

    pid, tgid, state, nvcsw, nivcsw = fields[:5]
    if pid != tgid:
        print(f'Thread {pid} - Process {tgid}:')
    else:
        print(f'Process {pid}:')
    print('\t* State:', state)
    print('\t* Number of Voluntary Context Switches:', nvcsw)
    print('\t* Number of Involuntary Context Switches:', nivcsw)
    print(f'\t* Open Files ({len(fields) - 5}):')
    for item in fields[5:]:
        fd, path = item.split(',')
        print(f'\t\t+ fd: {fd} - File: {path}')

    time.sleep(args.period)
