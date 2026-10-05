def crivo(n):
    primo = [True] * (n + 1)
    primo[0] = primo[1] = False
    i = 2
    while i * i <= n:
        if primo[i]:
            for j in range(i * i, n + 1, i):
                primo[j] = False
        i += 1
    return sum(1 for p in primo if p)

total = 0
for _ in range(10):
    total = crivo(1000000)
print(total)
