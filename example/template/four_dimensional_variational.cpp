#define VERDANDI_DEBUG_LEVEL_4
#define SELDON_WITH_BLAS
#define SELDON_WITH_LAPACK

#define VERDANDI_WITH_ABORT
#define VERDANDI_DENSE

#define VERDANDI_WITH_TRAJECTORY_MANAGER

//#define VERDANDI_WITH_DIRECT_SOLVER
//#define SELDON_WITH_MUMPS

#include "Verdandi.hxx"

#include "seldon/SeldonSolver.hxx"
#include "seldon/computation/optimization/NLoptSolver.cxx"

#include "model/ModelTemplate.cxx"
#include "observation_manager/ObservationManagerTemplate.cxx"
#include "method/FourDimensionalVariational.cxx"


int main(int argc, char** argv)
{

    VERDANDI_TRY;

    if (argc != 2)
    {
        string mesg  = "Usage:\n";
        mesg += string("  ") + argv[0] + " [configuration file]";
        std::cout << mesg << std::endl;
        return 1;
    }


    Verdandi::FourDimensionalVariational<double,
        Verdandi::ModelTemplate,
        Verdandi::ObservationManagerTemplate,
        Seldon::NLoptSolver> driver;

    driver.Initialize(argv[1]);

    driver.Analyze();

    while (!driver.HasFinished())
    {
        driver.InitializeStep();
        driver.Forward();
        driver.FinalizeStep();
    }

    driver.Finalize();

    VERDANDI_END;

    return 0;

}
