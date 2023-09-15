import time

def fib(n):
    if n < 2: return n
    return fib(n - 2) + fib(n - 1)

start = time.monotonic_ns()
print("fib(35) = {}, elapsed time = {}".format(fib(35), (time.monotonic_ns() - start) * 1e-9))