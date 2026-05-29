#ifndef MODEL_HPP
#define MODEL_HPP

/* TouchGFX 4.26.1: no MVPModel base — Model is a plain class (template parameter) */
class ModelListener {};

class Model
{
public:
    Model() {}
    void bind(ModelListener* listener) { modelListener = listener; }

protected:
    ModelListener* modelListener = nullptr;
};

#endif
