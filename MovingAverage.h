#include "CircularBuffer.h"

// Clase auxiliar para manejar una media móvil.

class MovingAverage {
  public:
  MovingAverage (const int size);
  float Add (const float newValue);

  private:
  CircularBuffer<float> avg;
};

MovingAverage::MovingAverage (const int size) :avg(size) {
}

float MovingAverage::Add (const float newValue) {
  avg.Add(newValue);

  float sum = 0;
  for (int i = 0; i < avg.NumberOfElements(); i++)
    sum += avg[i];

  return sum / avg.NumberOfElements();
}