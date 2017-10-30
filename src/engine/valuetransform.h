
#pragma once

#include <functional>

class NumberTransform {
public:
    static TRef<Number> Add(Number* a, Number* b);
    static TRef<Number> Subtract(Number* a, Number* b);
    static TRef<Number> Divide(Number* a, Number* b);
    static TRef<Number> Multiply(Number* a, Number* b);
    static TRef<Number> Min(Number* a, Number* b);
    static TRef<Number> Max(Number* a, Number* b);
    static TRef<Number> Mod(Number* a, Number* b);

    static TRef<Number> Round(Number* a, int decimals);
    static TRef<Number> Sin(Number* a);
    static TRef<Number> Cos(Number* a);
};

class PointTransform {
public:
    static TRef<PointValue> Create(Number* a, Number* b);

    static TRef<Number> X(PointValue* point);
    static TRef<Number> Y(PointValue* point);
};

template<class TransformedType, class OriginalType>
class TransformedValue : public TStaticValue<TransformedType> {

    std::function<TransformedType(OriginalType)> m_callback;

protected:
    TransformedType GetEvaluatedValue(OriginalType value) {
        return m_callback(value);
    }

public:
    TransformedValue(std::function<TransformedType(OriginalType)> callback, TStaticValue<OriginalType>* value) :
        m_callback(callback),
        TStaticValue(value)
    {}

    void Evaluate()
    {
        OriginalType value = ((TStaticValue<OriginalType>*)GetChild(0))->GetValue();

        TransformedType evaluated = GetEvaluatedValue(value);

        GetValueInternal() = evaluated;
    }
};

template<class TransformedType, class OriginalType, class OriginalType2>
class TransformedValue2 : public TStaticValue<TransformedType> {
    typedef std::function<TransformedType(OriginalType, OriginalType2)> CallbackType;
    CallbackType m_callback;

protected:
    TransformedType GetEvaluatedValue(OriginalType value, OriginalType2 value2) {
        return m_callback(value, value2);
    }

public:
    TransformedValue2(CallbackType callback, TStaticValue<OriginalType>* value, TStaticValue<OriginalType2>* value2) :
        m_callback(callback),
        TStaticValue(value, value2)
    {}

    void Evaluate()
    {
        OriginalType value = ((TStaticValue<OriginalType>*)GetChild(0))->GetValue();
        OriginalType2 value2 = ((TStaticValue<OriginalType2>*)GetChild(1))->GetValue();

        TransformedType evaluated = GetEvaluatedValue(value, value2);

        GetValueInternal() = evaluated;
    }
};