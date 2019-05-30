// Clase auxiliar para representar los datos del sensor y su situación.

class SensorData {
  public:
  SensorData (int s, int p);
  void SetSensorValue (int value);
  int GetSensorValue () const;
  void SetPotValue (int value);
  int GetPotValue () const;
  bool GetAlarm () const;
  
  private:
  int valSensor;
  int valPot;
};

SensorData::SensorData (int s = 0, int p = 0) {
  SetSensorValue(s);
  SetPotValue(p);
}

void SensorData::SetSensorValue (int value) {
  valSensor = value;
}

int SensorData::GetSensorValue () const {
  return valSensor;
}

void SensorData::SetPotValue (int value) {
  valPot = value;
}

int SensorData::GetPotValue () const {
  return valPot;
}

bool SensorData::GetAlarm () const {
  return (valSensor > valPot);
}

bool operator== (const SensorData& left, const SensorData& right) {
  if (left.GetSensorValue() != right.GetSensorValue())
    return false;
  if (left.GetPotValue() != right.GetPotValue())
    return false;
  if (left.GetAlarm() != right.GetAlarm())
    return false;

  return true;
}

bool operator!= (const SensorData& left, const SensorData& right) {
  return !(left == right);
}