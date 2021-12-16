def print_cpu_info():
    with open('/proc/cpuinfo') as f:
        cpus = f.read().rstrip().split('\n\n')
        print(cpus[-1])


if __name__ == '__main__':
    print_cpu_info()
