#include "IValue.h"

void IValue::setShadow(void* addr) {
  shadow = addr;
}

void IValue::setLineNumber(int l) {
  lineNumber = l;
}

void IValue::setType(KIND t) {
  type = t;
}

void IValue::setValue(VALUE v) {
  value = v;
}

void IValue::setValueOffset(int64_t v) {
  valueOffset = v;
}

void IValue::setScope(SCOPE s) {
  scope = s;
}

void IValue::setSize(unsigned int s) {
  size = s;
}

void IValue::setOffset(int o) {
  offset = o;
}

void IValue::setBitOffset(int bo) {
  bitOffset = bo;
}

void IValue::setIndex(unsigned i) {
  index = i;
}

void IValue::setFirstByte(unsigned f) {
  firstByte = f;
}

void IValue::setLength(unsigned l) {
  length = l;
}

int IValue::getLineNumber() {
  return lineNumber;
}

void* IValue::getShadow() {
  return shadow;
}

KIND IValue::getType() {
  return type;
}

VALUE IValue::getValue() {
  return value;
}

int64_t IValue::getValueOffset() {
  return valueOffset;
}

SCOPE IValue::getScope() {
  return scope;
}

unsigned int IValue::getSize() {
  return size;
}

int IValue::getOffset() {
  return offset;
}

int IValue::getBitOffset() {
  return bitOffset;
}

unsigned IValue::getIndex() {
  return index;
}

unsigned IValue::getFirstByte() {
  return firstByte;
}

unsigned IValue::getLength() {
  return length;
}

string IValue::toString() {
  std::stringstream s;

  switch(scope) {
  case GLOBAL:
    s << "GLOBAL   => ";
    break;
  case LOCAL:
    s << "LOCAL    => ";
    break;
  case REGISTER:
    s << "REGISTER => ";
    break;
  default:
    safe_assert(false);
  }

  switch(type) {
  case PTR_KIND:
    s << "[PTR: " << value.as_ptr << "]";
    break;
  case ARRAY_KIND:
    s << "[ARRAY: " << value.as_ptr << "]";
    break;
  case STRUCT_KIND:
    s << "[STRUCT: " << value.as_ptr << "]";
    break;
  case INT1_KIND:
    s << "[INT1: " << value.as_int << "] ";
    s << ", bitOffset: " << bitOffset;
    break;
  case INT8_KIND:
    s << "[INT8: " << value.as_int << "]";
    break;
  case INT16_KIND:
    s << "[INT16: " << value.as_int << "]";
    break;
  case INT32_KIND:
    s << "[INT32: " << value.as_int << "]";
    break;
  case INT64_KIND:
    s << "[INT64: " << value.as_int << "]";
    break;
  case FLP32_KIND:
    // TODO: this assumes value.as_flp returns a long double value
    s << "[FLP32: " << (float)value.as_flp << "]";
    break;
  case FLP64_KIND:
    // TODO: this assumes value.as_flp returns a long double value
    s << "[FLP64: " << (double) value.as_flp << "]";
    break;
  case FLP80X86_KIND:
    // TODO: this is incorrect because value.as_flp returns a double
    s << "[FLP80X86: " << (long double) value.as_flp << "]";
    break;
  case VOID_KIND:
    s << "[VOID]";
    break;
  default: 
    cout << "[IValue::toString] => Unhandled type" << endl;
    safe_assert(false);
    break;
  }

  s << ", Size: " << size;
  s << ", Offset: " << offset; 
  s << ", Index: " << index;
  s << ", Line: " << lineNumber;
  s << ", FirstByte: " << firstByte;
  s << ", Length: " << length;
  s << ", Initialized: " << isInitialized();

  return s.str();
}

void IValue::copy(IValue *dest) {
  dest->setType(type);
  dest->setValue(value);
  dest->setSize(size);
  dest->setOffset(offset);
  dest->setIndex(index);
  dest->setLength(length);
  // dest->setScope(scope); // I don't think the scope should be propagated
  // note: we do never overwrite the field firstByte
}

VALUE IValue::readValue(int offset, KIND type) {
//  cout << "readValue KIND: " << KIND_ToString(type) << endl;
//  cout << toString() << endl;


  IValue* valueArray = static_cast<IValue*>(value.as_ptr);
//  cout << "valueArray[index]: " <<  valueArray[index].toString() << endl;

  int byte = KIND_GetSize(type);
  VALUE value;

  if (offset == 0 && KIND_GetSize(valueArray[index].getType()) == byte) {
    // efficient code for common cases
    value = valueArray[index].getValue();
//    cout << "value: " << value.as_int << endl;
  } else {
    // uncommon cases
    unsigned nextIndex = index;
    int totalByte = 0;

    // TODO: review the condition nextIndex < length
    while (totalByte < offset + byte && nextIndex < length) {
      IValue value = valueArray[nextIndex];
      totalByte += KIND_GetSize(value.getType());
      nextIndex++;
    }

    // totalContent stores the accumulative content from IValue at index 
    // to IValue at nextIndex-1
    uint8_t* totalContent = (uint8_t*) malloc(totalByte*sizeof(uint8_t));
    int tocInx = 0;

    for (unsigned i = index; i < nextIndex; i++) {
      IValue value = valueArray[i];

//      cout << "\t [IValue::readValue] value: " << value.toString() << endl;

      KIND type = value.getType();
      int size = KIND_GetSize(type);
      VALUE valValue = value.getValue();
      uint8_t* valueContent = (uint8_t*) &valValue;
      for (int j = 0; j < size; j++) {
        totalContent[tocInx] = valueContent[j];
        tocInx++;
      }
    }

//    cout << "\t [IValue::readvalue] value in double: " << ((double*) totalContent)[0] << endl;

    // truncate content from total content
    uint8_t* truncContent = (uint8_t*) calloc(8, sizeof(uint8_t)); // TODO: magic number 8
    int trcInx = 0;

    for (int i = offset; i < offset + byte; i++) {
//      cout << "\t [IValue::readvalue] i: " << i << endl;
      truncContent[trcInx] = totalContent[i];
      trcInx++;
    }

    // cast truncate content array to an actual value 
    switch(type) {
    case FLP32_KIND: {
      float* truncValue = (float*) truncContent;
//      cout << "\t [IValue:readvalue] final value: " << truncValue[0] << endl;
      value.as_flp = *truncValue;
      break;
    }
    case FLP64_KIND: {
      double* truncValue = (double*) truncContent;
//      cout << "\t [IValue:readvalue] final value: " << truncValue[0] << endl;
      value.as_flp = *truncValue;
      break;
    }
    default: {
      int64_t* truncValue = (int64_t*) truncContent;
//      cout << "\t [IValue:readvalue] final value: " << truncValue[0] << endl;
      value.as_int = *truncValue;
      break;
    }
    }

  }
//  cout << "value at the end: " << value.as_int << endl;

  return value;
}

int IValue::setValue(int offset, int byte, uint8_t* content) {
  uint8_t* valueBytes = (uint8_t*)&value;
  int maxOffset = KIND_GetSize(type)-1;
  if (byte > maxOffset - offset +1) {
    byte = maxOffset - offset + 1;
  }

//  cout << "=== setValue === " << byte << endl;

  for (int i = 0; i < byte; i++) {
//    cout << "=== at " << i + offset << " " << (int64_t) valueBytes[i+offset] << endl;
    valueBytes[i+offset] = content[i];
//    cout << "=== at " << i + offset << " " << (int64_t) content[i] << endl;
//    cout << "=== at " << i + offset << " " << (int64_t) valueBytes[i+offset] << endl;
  }

  return byte;
} 

void IValue::writeValue(int offset, int byte, IValue* src) {
  IValue* valueArray = static_cast<IValue*>(value.as_ptr);

  cout << "\t writing " << byte << " bytes\n" << endl;

  if (offset == 0 && KIND_GetSize(valueArray[index].getType()) == byte) {
    // efficient code for common case
    cout << "\t trivial write" << endl;
    src->copy(&valueArray[index]);
  } else {
    VALUE srcValue = src->getValue();

    // get content from source value
    uint8_t* srcContent = (uint8_t*)(&srcValue);
    uint8_t* content = (uint8_t*) malloc(byte*sizeof(uint8_t*));
    for (int i = 0; i < byte; i++) {
      content[i] = srcContent[i];
      cout << "=== content value " << i << " " << (int64_t) content[i] << endl;
    }

    // write the content to this value array
    int currentIndex = index;
    int byteWrittens = 0;
    int oldByteWrittens = 0;

    while (byteWrittens < byte) {
      IValue *currentValue = &valueArray[currentIndex];
      cout << "=== Ivalue: " << currentValue->toString() << endl;
      cout << "=== current content: " << (int64_t) *content << endl; 
      cout << "=== current value: " << currentValue->getValue().as_int << endl;
      byteWrittens += currentValue->setValue(offset, byte - byteWrittens, content);
      cout << "=== byteWrittens: " << byteWrittens << endl;
      cout << "=== current value after: " << currentValue->getValue().as_int << endl;
      cout << "=== Ivalue after: " << currentValue->toString() << endl;
      content += byteWrittens - oldByteWrittens;
      oldByteWrittens = byteWrittens;
      currentIndex++;
      offset = 0;
    }
  }
}

bool IValue::isInitialized() {
  return type != PTR_KIND || length > 0;
}

void IValue::setInitialized() {
  if (type == PTR_KIND && length == 0) {
    length = 1;
  }
}

bool IValue::isIValue(KIND t) {
  return type == t;
}

int64_t IValue::getIntValue() {
  int64_t v = value.as_int;
  switch (type) {
    case INT1_KIND:
      return (bool) v;
    case INT8_KIND:
      return (int8_t) v;
    case INT16_KIND:
      return (int16_t) v;
    case INT32_KIND:
      return (int32_t) v;
    case INT64_KIND:
      return (int64_t) v;
    default:
      return v;
  }
}

void* IValue::getPtrValue() {
  return value.as_ptr;
}

void* IValue::getIPtrValue() {
  return (void*)((int64_t)value.as_ptr + valueOffset);
}

double IValue::getFlpValue() {
  double v = value.as_flp;
  switch (type) {
    case FLP32_KIND:
      return (float) v;
    case FLP64_KIND:
      return (double) v;
    default:
      return v;
  }
}
