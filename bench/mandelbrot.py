def pertence(cx, cy):
    x = 0.0
    y = 0.0
    for _ in range(200):
        x2 = x * x
        y2 = y * y
        if x2 + y2 > 4.0:
            return False
        y = 2.0 * x * y + cy
        x = x2 - y2 + cx
    return True

total = 0
for py in range(600):
    for px in range(800):
        cx = -2.0 + 3.0 * px / 800
        cy = -1.2 + 2.4 * py / 600
        if pertence(cx, cy):
            total += 1
print(total)
