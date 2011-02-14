#define SELDON_DEBUG_LEVEL_4
#define SELDON_WITH_BLAS
#define SELDON_WITH_LAPACK

#define VERDANDI_WITH_ABORT
#define VERDANDI_DENSE

#define VERDANDI_WITH_DIRECT_SOLVER
#define SELDON_WITH_MUMPS

#include "Verdandi.hxx"

#include "seldon/SeldonSolver.hxx"
#include "seldon/computation/optimization/NLoptSolver.cxx"

#include "model/ClampedBar.cxx"
#include "observation_manager/LinearObservationManager.cxx"
#include "method/FourDimensionalVariational.cxx"


int main(int argc, char** argv)
{

    TRY;

    if (argc != 2)
    {
        string mesg  = "Usage:\n";
        mesg += string("  ") + argv[0] + " [configuration file]";
        std::cout << mesg << std::endl;
        return 1;
    }

    typedef double real;

    Verdandi::FourDimensionalVariational<real,
        Verdandi::ClampedBar<real>,
        Verdandi::LinearObservationManager<real>,
        Seldon::NLoptSolver> driver(argv[1]);

    driver.Initialize();

    driver.Analyze();

    while (!driver.HasFinished())
    {
        driver.InitializeStep();

        driver.Forward();
    }

    END;

    return 0;

}