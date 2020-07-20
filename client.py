import argparse
import time

FILE_FLAG = '1'
USER_FLAG = '0'
DEVICE_FILE = '/dev/phase2'
parser = argparse.ArgumentParser(description='Phase2')
parser.add_argument('--devfile', default=DEVICE_FILE,
                    type=str, help='driver path')
args = parser.parse_args()



def add_entery(flag):
    def wrapper():
        global args, FILE_FLAG
        path = input('\tenter file path: ' if flag == FILE_FLAG else '\tenter user id: ')
        res = int(input('\tenter security level (0-3): '))
        if not 0 <= res <= 3:
            print('invalid security level')
            return
        with open(args.devfile, 'w') as f:
            f.write(f'{flag}{res}{path}\n')
    return wrapper

def print_list():
    result = None
    with open(args.devfile, 'r', errors='ignore') as f:
        result = f.readline()
    names = result[:len(result)-1].split(':')
    if names and names[0] != 'users':
        print('- files: ')
    for name in names:
        if name:
            if name == 'users':
                print('- users: ')
                continue
            print(f'\t* {name[1:]} - {name[0]}')

commands = {
    'exit': lambda: exit(),
    'file': add_entery(FILE_FLAG),
    'user': add_entery(USER_FLAG),
    'list': print_list,
}


while True:
    cmd = input('enter command (user/file/list/exit): ')
    for key, f in commands.items():
        if key == cmd:
            f()
            break
