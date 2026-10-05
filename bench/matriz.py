n = 400
a = [[(i + j) / n for j in range(n)] for i in range(n)]
b = [[(i + 2 * j) / n for j in range(n)] for i in range(n)]
c = [[0.0] * n for _ in range(n)]
for i in range(n):
    for j in range(n):
        soma = 0.0
        for k in range(n):
            soma += a[i][k] * b[k][j]
        c[i][j] = soma
traco = 0.0
for i in range(n):
    traco += c[i][i]
print(int(traco))
