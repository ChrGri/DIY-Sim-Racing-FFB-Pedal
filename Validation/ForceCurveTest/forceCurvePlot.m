close all; clear all; clc;

% --- Hauptskript ---
% 1. Definition der Kontrollpunkte
x = [0, 20, 40, 60, 80, 100];
y = [0, 10, 20, 30, 50, 100];

% 2. Koeffizienten mit DEINEM C++ Algorithmus berechnen
[a, b] = fitMatrix_cpp(x, y);

% 3. Fein aufgelöste x-Werte für die Auswertung
xx = linspace(min(x), max(x), 1000);
yy = zeros(size(xx));
dydx = zeros(size(xx));

% 4. Kurve und Gradient evaluieren (wie in EvalForceCubicSpline)
nPoints = length(x);
for j = 1:length(xx)
    xi = xx(j);
    
    % Segment finden (entspricht der Schleife in deinem C++ Code)
    seg = 1;
    for i = 1:(nPoints - 2)
        if xi <= x(i+1)
            break;
        end
        seg = seg + 1;
    end
    
    % Lokaler Parameter t und dynamisches dx berechnen
    dx = x(seg+1) - x(seg);
    t = (xi - x(seg)) / dx;
    t = max(0, min(1, t)); % Sicherheitshalber auf [0, 1] begrenzen
    
    % Position y berechnen (deine C++ Formel)
    yy(j) = (1 - t)*y(seg) + t*y(seg+1) + t*(1 - t)*(a(seg)*(1 - t) + b(seg)*t);
    
    % Gradient dy/dx berechnen (die abgeleitete C++ Formel)
    dy = y(seg+1) - y(seg);
    yPrime_t = dy + (1 - 2*t)*(a(seg)*(1 - t) + b(seg)*t) + t*(1 - t)*(b(seg) - a(seg));
    dydx(j) = yPrime_t / dx;
end

% 5. Plotten der Ergebnisse
figure('Name', 'C++ Spline Algorithmus', 'Position', [100, 100, 800, 600]);

subplot(2,1,1);
plot(xx, yy, 'b-', 'LineWidth', 2); hold on;
plot(x, y, 'ro', 'MarkerSize', 8, 'MarkerFaceColor', 'r');
grid on;
title('Position: Deine C++ Spline-Interpolation');
xlabel('x (%)'); ylabel('y');
legend('Spline-Kurve', 'Kontrollpunkte', 'Location', 'NorthWest');

subplot(2,1,2);
plot(xx, dydx, 'g-', 'LineWidth', 2); hold on;
grid on;
title('Gradient: Deine C++ Berechnung');
xlabel('x (%)'); ylabel('Gradient (dy/dx)');

% Markierungslinien bei den Kontrollpunkten zur Orientierung
for i = 1:length(x)
    xline(x(i), 'k--', 'Alpha', 0.2);
end


% --- Funktionen ---

% Dies ist eine exakte 1:1 MATLAB-Übersetzung deiner "fitMatrix" Funktion.
% Da MATLAB 1-basiert iteriert (statt 0-basiert wie C++), sind die Indizes um +1 verschoben.
function [a, b] = fitMatrix_cpp(x, y)
    numPoints = length(x);
    matrixA = zeros(1, numPoints);
    matrixB = zeros(1, numPoints);
    matrixC = zeros(1, numPoints);
    r = zeros(1, numPoints);

    dx1 = x(2) - x(1);
    matrixC(1) = 1.0 / dx1;
    matrixB(1) = 2.0 * matrixC(1);
    r(1) = 3.0 * (y(2) - y(1)) / (dx1 * dx1);

    for i = 2:(numPoints - 1)
        dx1 = x(i) - x(i-1);
        dx2 = x(i+1) - x(i);
        matrixA(i) = 1.0 / dx1;
        matrixC(i) = 1.0 / dx2;
        matrixB(i) = 2.0 * (matrixA(i) + matrixC(i));
        dy1 = y(i) - y(i-1);
        dy2 = y(i+1) - y(i);
        r(i) = 3.0 * (dy1 / (dx1 * dx1) + dy2 / (dx2 * dx2));
    end

    dx1 = x(numPoints) - x(numPoints - 1);
    dy1 = y(numPoints) - y(numPoints - 1);
    matrixA(numPoints) = 1.0 / dx1;
    matrixB(numPoints) = 2.0 * matrixA(numPoints);
    r(numPoints) = 3.0 * (dy1 / (dx1 * dx1));

    cPrime = zeros(1, numPoints);
    dPrime = zeros(1, numPoints);
    k = zeros(1, numPoints);

    cPrime(1) = matrixC(1) / matrixB(1);
    for i = 2:numPoints
        cPrime(i) = matrixC(i) / (matrixB(i) - cPrime(i-1) * matrixA(i));
    end

    dPrime(1) = r(1) / matrixB(1);
    for i = 2:numPoints
        dPrime(i) = (r(i) - dPrime(i-1) * matrixA(i)) / (matrixB(i) - cPrime(i-1) * matrixA(i));
    end

    k(numPoints) = dPrime(numPoints);
    for i = (numPoints - 1):-1:1
        k(i) = dPrime(i) - cPrime(i) * k(i+1);
    end

    a = zeros(1, numPoints - 1);
    b = zeros(1, numPoints - 1);
    for i = 1:(numPoints - 1)
        dx1 = x(i+1) - x(i);
        dy1 = y(i+1) - y(i);
        a(i) = k(i) * dx1 - dy1;
        b(i) = -k(i+1) * dx1 + dy1;
    end
end