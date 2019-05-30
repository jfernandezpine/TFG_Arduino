// Clase auxiliar para manejar un buffer circular.

template <class T>
class CircularBuffer {
  public:
  CircularBuffer (const int size);
  ~CircularBuffer ();
  void Add (const T& value);
  int NumberOfElements () const;
  T& operator[] (int index) const;

  private:
  int maxSize;
  int firstPos;
  int numberOfElements;
  T* buffer;
};

template <class T>
CircularBuffer<T>::CircularBuffer (const int size) {
  maxSize = size;
  firstPos = numberOfElements = 0;
  buffer = new T[maxSize];
}

template <class T>
CircularBuffer<T>::~CircularBuffer () {
    delete buffer;
}

template <class T>
void CircularBuffer<T>::Add (const T& value) {
  // Calculamos la posición de inserción.
  int firstFreePos = (firstPos + numberOfElements) % maxSize;

  // Insertamos el valor.
  buffer[firstFreePos] = value;

  // El primer elemento del buffer se desplaza si este
  // ya se ha llenado. En caso contrario no hace falta
  // descartarlo.
  if (numberOfElements >= maxSize)
    firstPos++;
  else
    numberOfElements++;  
}

template <class T>
int CircularBuffer<T>::NumberOfElements () const {
  return numberOfElements;
}

template <class T>
T& CircularBuffer<T>::operator[] (int index) const {
  // Transformamos el posicionamiento de index en el rango
  // 0 .. N-1 a las posiciones en el buffer circular.
  int pos = (firstPos + index) % maxSize;

  // Devolvemos el valor.
  return buffer[pos];
}
