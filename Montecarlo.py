import numpy as np

def f(x, y, z):
    sigma = 1
    return np.exp(-(x**2 + y**2 + z**2) / (2 * sigma**2))

def integrand(x, y, z):
    return np.cos(x * y * z)

def monte_carlo_integration(num_samples):
    integral_sum = 0

    for _ in range(num_samples):
        x = np.random.uniform(-np.pi/2, np.pi/2)
        y = np.random.uniform(-np.pi/2, np.pi/2)
        z = np.random.uniform(-np.pi/2, np.pi/2)

        integrand_value = integrand(x, y, z)
        weight = f(x, y, z)
        
        integral_sum += integrand_value / weight

    average_integral = integral_sum / num_samples
    volume = (np.pi/2)**3
    estimated_integral = average_integral * volume

    return estimated_integral

# Number of Monte Carlo samples
num_samples = 1000000
# Estimate the integral
result = monte_carlo_integration(num_samples)
print("Estimated Integral:", result)
