// Copyright (C) 2008-2009 INRIA
// Author(s): Vivien Mallet, Claire Mouton
//
// This file is part of the data assimilation library Verdandi.
//
// Verdandi is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your option)
// any later version.
//
// Verdandi is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
// more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Verdandi. If not, see http://www.gnu.org/licenses/.


#ifndef VERDANDI_FILE_OPTIMALINTERPOLATION_CXX


#include "OptimalInterpolation.hxx"


namespace Verdandi
{


    /////////////////////////////////
    // CONSTRUCTORS AND DESTRUCTOR //
    /////////////////////////////////


    //! Main constructor.
    /*! Builds the driver and reads option keys in the configuration file.
      \param[in] configuration_file configuration file.
    */
    template <class T, class ClassModel, class ClassObservationManager>
    OptimalInterpolation<T, ClassModel, ClassObservationManager>
    ::OptimalInterpolation(string configuration_file):
        model_(configuration_file),
        observation_manager_(model_, configuration_file)
    {
        Ops configuration(configuration_file);

        /*** Initializations ***/

        MessageHandler::AddRecipient("model", model_,
                                     ClassModel::StaticMessage);
        MessageHandler::AddRecipient("observation_manager",
                                     observation_manager_,
                                     ClassObservationManager::StaticMessage);
        MessageHandler::AddRecipient("driver", *this,
                                     OptimalInterpolation::StaticMessage);


        /***************************
         * Reads the configuration *
         ***************************/


        /*** Display options ***/

        configuration.SetPrefix("optimal_interpolation.display.");
        // Should iterations be displayed on screen?
        configuration.Set("show_iteration",
                          option_display_["show_iteration"]);
        // Should current time be displayed on screen?
        configuration.Set("show_time", option_display_["show_time"]);

        /*** Assimilation options ***/

        configuration.
            SetPrefix("optimal_interpolation.data_assimilation.");
        configuration.Set("analyze_first_step", analyze_first_step_);

        configuration.SetPrefix("optimal_interpolation.");
        configuration.Set("BLUE_computation",
                          "ops_in(v, {'vector', 'matrix'})",
                          blue_computation_);

        /*** Ouput saver ***/

        configuration.SetPrefix("optimal_interpolation.output_saver.");
        output_saver_.Initialize(configuration);
        output_saver_.Empty("state_forecast");
        output_saver_.Empty("state_analysis");

        /*** Logger and read configuration ***/

        configuration.SetPrefix("optimal_interpolation.");

        if (configuration.Exists("output.log"))
            Logger::SetFileName(configuration.Get<string>("output.log"));

        if (configuration.Exists("output.configuration"))
        {
            string output_configuration;
            configuration.Set("output.configuration", output_configuration);
            configuration.WriteLuaDefinition(output_configuration);
        }
    }


    //! Destructor.
    template <class T, class ClassModel, class ClassObservationManager>
    OptimalInterpolation<T, ClassModel, ClassObservationManager>
    ::~OptimalInterpolation()
    {
    }


    /////////////
    // METHODS //
    /////////////


    //! Initializes the optimal interpolation driver.
    /*! Initializes the model and the observation manager. Optionally computes
      the analysis of the first step.
      \param[in] configuration_file configuration file.
    */
    template <class T, class ClassModel, class ClassObservationManager>
    void OptimalInterpolation<T, ClassModel, ClassObservationManager>
    ::Initialize(string configuration_file)
    {
        MessageHandler::Send(*this, "all", "::Initialize begin");

        /*** Initializations ***/

        model_.Initialize(configuration_file);
        observation_manager_.Initialize(model_, configuration_file);

        /*** Assimilation ***/

        if (analyze_first_step_)
            Analyze();

        MessageHandler::Send(*this, "model", "initial condition");

        MessageHandler::Send(*this, "all", "::Initialize end");
    }


    //! Initializes a step for the optimal interpolation.
    /*! Initializes a step for the model.
     */
    template <class T, class ClassModel, class ClassObservationManager>
    void OptimalInterpolation<T, ClassModel, ClassObservationManager>
    ::InitializeStep()
    {
        MessageHandler::Send(*this, "all", "::InitializeStep begin");

        if (option_display_["show_time"])
            cout << "Current step: "
                 << model_.GetTime() << endl;
        model_.InitializeStep();

        MessageHandler::Send(*this, "all", "::InitializeStep end");
    }


    //! Performs a step forward, with optimal interpolation at the end.
    template <class T, class ClassModel, class ClassObservationManager>
    void OptimalInterpolation<T, ClassModel, ClassObservationManager>
    ::Forward()
    {
        MessageHandler::Send(*this, "all", "::Forward begin");

        model_.Forward();

        MessageHandler::Send(*this, "model", "forecast");
        MessageHandler::Send(*this, "observation_manager", "forecast");
        MessageHandler::Send(*this, "driver", "forecast");


        MessageHandler::Send(*this, "all", "::Forward end");
    }


    //! Computes an analysis.
    /*! Whenever observations are available, it computes BLUE.
     */
    template <class T, class ClassModel, class ClassObservationManager>
    void OptimalInterpolation<T, ClassModel, ClassObservationManager>
    ::Analyze()
    {
        MessageHandler::Send(*this, "all", "::Analyze begin");

        observation_manager_.SetTime(model_, model_.GetTime());

        if (observation_manager_.HasObservation())
        {
            if (option_display_["show_time"])
                cout << "Performing optimal interpolation at time step ["
                     << model_.GetTime() << "]..." << endl;

            model_state state;
            model_.GetState(state);
            Nstate_ = model_.GetNstate();

            observation innovation;
            observation_manager_.GetInnovation(state, innovation);
            Nobservation_ = innovation.GetSize();

            ComputeBLUE(innovation, state);

            model_.SetState(state);

            if (option_display_["show_time"])
                cout << " done." << endl;

            MessageHandler::Send(*this, "model", "analysis");
            MessageHandler::Send(*this, "observation_manager", "analysis");
            MessageHandler::Send(*this, "driver", "analysis");
        }

        MessageHandler::Send(*this, "all", "::Analyze end");
    }


    //! Computes BLUE for optimal interpolation.
    /*! The state is updated by the combination of background state and
      innovation. It computes the BLUE (best linear unbiased estimator).
      \param[in] innovation the innovation vector.
      \param[in] state the state vector to analyze.
    */
    template <class T, class ClassModel, class ClassObservationManager>
    void OptimalInterpolation<T, ClassModel, ClassObservationManager>
    ::ComputeBLUE(const observation& innovation, model_state& state)
    {
        if (blue_computation_ == "vector")
            ComputeBLUE_vector(innovation, state);
        else
            ComputeBLUE_matrix(innovation, state);
    }


    //! Computes BLUE for optimal interpolation with dense matrices.
    /*!
      \param[in] innovation the innovation vector.
      \param[in] state the state vector to analyze.
    */
    template <class T, class ClassModel, class ClassObservationManager>
    void OptimalInterpolation<T, ClassModel, ClassObservationManager>
    ::ComputeBLUE_vector(const observation& innovation,
                         model_state& state)
    {
#ifdef VERDANDI_DENSE
        Nobservation_ = observation_manager_.GetNobservation();

        int r, c;

        // One row of background matrix B.
        model_state_error_variance_row error_covariance_row(Nstate_);

        // One row of tangent operator matrix.
        observation_tangent_linear_operator_row tangent_operator_row(Nstate_);

        // Temporary matrix and vector.
        // 'HBHR_inv' will eventually contain the matrix (HBH' + R)^(-1).
        Matrix<T> HBHR_inv(Nobservation_, Nobservation_);
        HBHR_inv.Fill(T(0));

        Vector<T> row(Nobservation_);

        // Computes HBH'.
        T H_entry;
        for (int j = 0; j < Nstate_; j++)
        {
            model_.GetStateErrorVarianceRow(j, error_covariance_row);
            // Computes the j-th row of BH'.
            for (r = 0; r < Nobservation_; r++)
            {
                observation_manager_
                    .GetTangentLinearOperatorRow(r, tangent_operator_row);
                row(r) = DotProd(error_covariance_row, tangent_operator_row);
            }

            // Keeps on building HBH'.
            for (r = 0; r < Nobservation_; r++)
            {
                H_entry = observation_manager_.GetTangentLinearOperator(r, j);
                for (c = 0; c < Nobservation_; c++)
                    HBHR_inv(r, c) += H_entry * row(c);
            }
        }

        // Computes (HBH' + R).
        for (r = 0; r < Nobservation_; r++)
            for (c = 0; c < Nobservation_; c++)
                HBHR_inv(r, c) += observation_manager_.GetErrorVariance(r, c);

        // Computes (HBH' + R)^{-1}.
        GetInverse(HBHR_inv);

        // Computes HBHR_inv * innovation.
        Vector<T> HBHR_inv_innovation(Nobservation_);
        MltAdd(T(1), HBHR_inv, innovation, T(0), HBHR_inv_innovation);

        // Computes new state.
        Vector<T> BHt_row(Nobservation_);
        BHt_row.Fill(T(0));
        for (r = 0; r < Nstate_; r++)
        {
            // Computes the r-th row of BH'.
            model_.GetStateErrorVarianceRow(r, error_covariance_row);
            for (c = 0; c < Nobservation_; c++)
            {
                observation_manager_
                    .GetTangentLinearOperatorRow(c, tangent_operator_row);
                BHt_row(c) = DotProd(error_covariance_row,
                                     tangent_operator_row);
            }

            state(r) += DotProd(BHt_row, HBHR_inv_innovation);
        }
#else
        throw ErrorUndefined("OptimalInterpolation::ComputeBLUE_vector");
#endif
    }


    //! Computes BLUE using operations on matrices.
    /*! This method is mainly intended for cases where the covariance matrices
      are sparse matrices. Otherwise, the manipulation of the matrices may
      lead to unreasonable memory requirements and to high computational
      costs.
      \param[in] innovation the innovation vector.
      \param[in] state the state vector to analyze.
    */
    template <class T, class ClassModel, class ClassObservationManager>
    void OptimalInterpolation<T, ClassModel, ClassObservationManager>
    ::ComputeBLUE_matrix(const observation& innovation,
                         model_state& state)
    {
#ifdef VERDANDI_SPARSE
        Nobservation_ = observation_manager_.GetNobservation();

        // Temporary matrix and vector.
        matrix_state_observation working_matrix_so(Nstate_, Nobservation_);
        observation_tangent_linear_operator working_matrix_oo(Nobservation_,
                                                              Nobservation_);
        // Computes BH'.
        MltAdd(T(1), SeldonNoTrans,
               model_.GetStateErrorVariance(), SeldonTrans,
               observation_manager_.GetTangentLinearOperator(), T(0),
               working_matrix_so);

        // Computes HBH'.
        Mlt(observation_manager_.GetTangentLinearOperator(),
            working_matrix_so, working_matrix_oo);

        // Computes (HBH' + R).
        Add(T(1),
            observation_manager_.GetErrorVariance(), working_matrix_oo);

        // Computes x = (HBH' + R)^{-1} * innovation by solving the linear
        // system (HBH' + R) * x = innovation.
#if defined(SELDON_WITH_UMFPACK)
        MatrixUmfPack<T> matrix_super_lu;
#elif defined(SELDON_WITH_SUPERLU)
        MatrixSuperLU<T> matrix_super_lu;
#elif defined(SELDON_WITH_MUMPS)
        MatrixMumps<T> matrix_super_lu;
#endif
        GetLU(working_matrix_oo, matrix_super_lu);
        observation working_vector(Nobservation_);
        working_vector = innovation;
        SolveLU(matrix_super_lu, working_vector);
        MltAdd(T(1.), working_matrix_so, working_vector, T(1.), state);
#else
        throw ErrorUndefined("OptimalInterpolation::ComputeBLUE_matrix");
#endif
    }


    //! Checks whether the model has finished.
    /*!
      \return True if no more data assimilation is required, false otherwise.
    */
    template <class T, class ClassModel, class ClassObservationManager>
    bool OptimalInterpolation<T, ClassModel, ClassObservationManager>
    ::HasFinished() const
    {
        return model_.HasFinished();
    }


    //! Returns the model.
    /*!
      \return The model.
    */
    template <class T, class ClassModel, class ClassObservationManager>
    const ClassModel&
    OptimalInterpolation<T, ClassModel, ClassObservationManager>
    ::GetModel() const
    {
        return model_;
    }


    //! Returns the name of the class.
    /*!
      \return The name of the class.
    */
    template <class T, class ClassModel, class ClassObservationManager>
    string OptimalInterpolation<T, ClassModel, ClassObservationManager>
    ::GetName() const
    {
        return "OptimalInterpolation";
    }


    //! Receives and handles a message.
    /*
      \param[in] message the received message.
    */
    template <class T, class ClassModel, class ClassObservationManager>
    void OptimalInterpolation<T, ClassModel, ClassObservationManager>
    ::Message(string message)
    {
        model_state state;
        if (message.find("forecast") != string::npos)
        {
            model_.GetState(state);
            output_saver_.Save(state, model_.GetTime(), "state_forecast");
        }

        if (message.find("analysis") != string::npos)
        {
            model_.GetState(state);
            output_saver_.Save(state, model_.GetTime(), "state_analysis");
        }

    }


} // namespace Verdandi.


#define VERDANDI_FILE_OPTIMALINTERPOLATION_CXX
#endif
