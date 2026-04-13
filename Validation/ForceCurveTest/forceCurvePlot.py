import numpy as np
import matplotlib.pyplot as plt
import os

# --- Functions ---

def fitMatrix_cpp(x, y):
    """
    Exact 1:1 Python translation of your C++ "fitMatrix" function.
    Accounts for Python's 0-based indexing vs C++'s 0-based indexing.
    """
    numPoints = len(x)
    matrixA = np.zeros(numPoints)
    matrixB = np.zeros(numPoints)
    matrixC = np.zeros(numPoints)
    r = np.zeros(numPoints)

    # First point boundary condition
    dx1 = x[1] - x[0]
    matrixC[0] = 1.0 / dx1
    matrixB[0] = 2.0 * matrixC[0]
    r[0] = 3.0 * (y[1] - y[0]) / (dx1 * dx1)

    # Inner points
    for i in range(1, numPoints - 1):
        dx1 = x[i] - x[i-1]
        dx2 = x[i+1] - x[i]
        matrixA[i] = 1.0 / dx1
        matrixC[i] = 1.0 / dx2
        matrixB[i] = 2.0 * (matrixA[i] + matrixC[i])
        dy1 = y[i] - y[i-1]
        dy2 = y[i+1] - y[i]
        r[i] = 3.0 * (dy1 / (dx1 * dx1) + dy2 / (dx2 * dx2))

    # Last point boundary condition (Natural Spline -> Clamped Fix depending on what you use)
    dx1 = x[numPoints-1] - x[numPoints-2]
    dy1 = y[numPoints-1] - y[numPoints-2]
    matrixA[numPoints-1] = 1.0 / dx1
    matrixB[numPoints-1] = 2.0 * matrixA[numPoints-1]
    r[numPoints-1] = 3.0 * (dy1 / (dx1 * dx1))

    cPrime = np.zeros(numPoints)
    dPrime = np.zeros(numPoints)
    k = np.zeros(numPoints)

    # Forward substitution
    cPrime[0] = matrixC[0] / matrixB[0]
    for i in range(1, numPoints):
        cPrime[i] = matrixC[i] / (matrixB[i] - cPrime[i-1] * matrixA[i])

    dPrime[0] = r[0] / matrixB[0]
    for i in range(1, numPoints):
        dPrime[i] = (r[i] - dPrime[i-1] * matrixA[i]) / (matrixB[i] - cPrime[i-1] * matrixA[i])

    # Backward substitution
    k[numPoints-1] = dPrime[numPoints-1]
    for i in range(numPoints - 2, -1, -1): # Counts backwards from numPoints-2 down to 0
        k[i] = dPrime[i] - cPrime[i] * k[i+1]

    # Calculate coefficients a and b
    a = np.zeros(numPoints - 1)
    b = np.zeros(numPoints - 1)
    for i in range(numPoints - 1):
        dx1 = x[i+1] - x[i]
        dy1 = y[i+1] - y[i]
        a[i] = k[i] * dx1 - dy1
        b[i] = -k[i+1] * dx1 + dy1

    return a, b


# --- Main Script ---

# 1. Define control points (from your test)
x = np.array([0, 20, 40, 60, 80, 100], dtype=float)
y = np.array([0, 10, 20, 30, 50, 100], dtype=float) # Note: 50 at x=80

# 2. Calculate coefficients
a, b = fitMatrix_cpp(x, y)

# 3. High resolution x-values for smooth evaluation
xx = np.linspace(min(x), max(x), 1000)
yy = np.zeros_like(xx)
dydx = np.zeros_like(xx)

# 4. Evaluate curve and gradient (replicating EvalForceCubicSpline)
nPoints = len(x)
for j in range(len(xx)):
    xi = xx[j]
    
    # Find active segment (0-based)
    seg = 0
    for i in range(nPoints - 2):
        if xi <= x[i+1]:
            break
        seg += 1
        
    # Calculate local parameter 't' and dynamic 'dx'
    dx = x[seg+1] - x[seg]
    t = (xi - x[seg]) / dx
    t = max(0.0, min(1.0, t)) # Constrain to [0, 1]
    
    # Calculate position y
    yy[j] = (1 - t)*y[seg] + t*y[seg+1] + t*(1 - t)*(a[seg]*(1 - t) + b[seg]*t)
    
    # Calculate gradient dy/dx
    dy = y[seg+1] - y[seg]
    yPrime_t = dy + (1 - 2*t)*(a[seg]*(1 - t) + b[seg]*t) + t*(1 - t)*(b[seg] - a[seg])
    dydx[j] = yPrime_t / dx

# 5. Load CSV Data (if file exists)
csv_filename = 'export.csv'
has_csv = os.path.exists(csv_filename)

if has_csv:
    try:
        # Load CSV. Adjust 'skiprows=1' if your CSV has a header line like "Travel,Force,Gradient"
        csv_data = np.loadtxt(csv_filename, delimiter=',', skiprows=0) 
        csv_x = csv_data[:, 0]
        csv_y = csv_data[:, 1]
        csv_grad = csv_data[:, 2]
        print(f"Successfully loaded {len(csv_x)} rows from {csv_filename}")
    except Exception as e:
        print(f"Error loading CSV: {e}")
        has_csv = False
else:
    print(f"File '{csv_filename}' not found in current directory. Plotting Python simulation only.")

# 6. Plot the results
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 10))

# Top Plot: Position
ax1.plot(xx, yy, 'b-', linewidth=2, label='Python Spline Simulation')
if has_csv:
    ax1.plot(csv_x, csv_y, 'k--', linewidth=2, alpha=0.7, label='C++ CSV Export (export.csv)')
ax1.plot(x, y, 'ro', markersize=8, markerfacecolor='r', label='Control Points')
ax1.set_title('Position: C++ Spline Interpolation Comparison')
ax1.set_xlabel('Travel (%)')
ax1.set_ylabel('Force')
ax1.grid(True)
ax1.legend(loc='upper left')

# Bottom Plot: Gradient
ax2.plot(xx, dydx, 'g-', linewidth=2, label='Python Gradient Simulation')
if has_csv:
    ax2.plot(csv_x, csv_grad, 'k--', linewidth=2, alpha=0.7, label='C++ CSV Export (export.csv)')
ax2.set_title('Gradient: Mathematical Derivative vs C++ Output')
ax2.set_xlabel('Travel (%)')
ax2.set_ylabel('Gradient (dy/dx)')
ax2.grid(True)
ax2.legend(loc='upper left')

# Vertical lines for control points in both plots
for xi in x:
    ax1.axvline(x=xi, color='k', linestyle='--', alpha=0.3)
    ax2.axvline(x=xi, color='k', linestyle='--', alpha=0.3)

plt.tight_layout()
plt.show()