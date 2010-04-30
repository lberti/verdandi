#define SELDON_DEBUG_LEVEL_4
#define VERDANDI_WITH_ABORT
#include "Verdandi.hxx"
using namespace Verdandi;

#include "ForwardDriver.cxx"

#include "QuadraticModel.cxx"


int main(int argc, char** argv)
{
    TRY;

    if (argc != 2)
    {
        string mesg  = "Usage:\n";
        mesg += string("  ") + argv[0] + " [configuration file]";
        cout << mesg << endl;
        return 1;
    }

    ForwardDriver<QuadraticModel<double> > driver(argv[1]);

    driver.Initialize(argv[1]);

    while (!driver.HasFinished())
    {
        driver.InitializeStep();
        driver.Forward();
    }

    END;

    return 0;
}