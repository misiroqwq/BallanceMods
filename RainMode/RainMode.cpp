#include "RainMode.h"
IMod* BMLEntry(IBML* bml) {
    return new RainMode(bml);
}
