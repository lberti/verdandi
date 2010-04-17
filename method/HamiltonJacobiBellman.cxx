// Copyright (C) 2009-2010 INRIA
// Author(s): Vivien Mallet, Serhiy Zhuk
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


#ifndef VERDANDI_FILE_HAMILTONJACOBIBELLMAN_CXX


#include "HamiltonJacobiBellman.hxx"


namespace Verdandi
{


    ////////////////////////////////
    // CONSTRUCTOR AND DESTRUCTOR //
    ////////////////////////////////


    //! Main constructor.
    /*! Builds the driver and reads option keys in the configuration file.
      \param[in] configuration_file configuration file.
    */
    template <class T, class ClassModel, class ClassObservationManager>
    HamiltonJacobiBellman<T, ClassModel, ClassObservationManager>
    ::HamiltonJacobiBellman(string configuration_file):
        model_(configuration_file),
        observation_manager_(model_, configuration_file),
        time_step_(0)
    {
        GetPot configuration_stream(configuration_file, "#", "\n");

        /*** Initializations ***/

        MessageHandler::AddRecipient("model", model_,
                                     ClassModel::StaticMessage);
        MessageHandler::AddRecipient("observation_manager",
                                     observation_manager_,
                                     ClassObservationManager::StaticMessage);
        MessageHandler::AddRecipient("driver", *this,
                                     HamiltonJacobiBellman::StaticMessage);


        /***************************
         * Reads the configuration *
         ***************************/


        /*** Display options ***/

        configuration_stream.set_prefix("HJB/display/");
        // Should iterations be displayed on screen?
        configuration_stream.set("Show_iteration",
                                 option_display_["show_iteration"]);
        // Should current date be displayed on screen?
        configuration_stream.set("Show_date", option_display_["show_date"]);

        /*** Domain definition ***/

        configuration_stream.set_prefix("HJB/domain/");

        // Discretization along all dimensions. A regular grid is assumed. In
        // every dimension, the format is x_min, delta_x, Nx for every
        // dimension.
        string discretization;
        configuration_stream.set("Discretization", discretization);
        vector<string> discretization_vector = split(discretization);
        if (discretization_vector.size() % 3 != 0)
            throw ErrorConfiguration("HamiltonJacobiBellman::"
                                     "HamiltonJacobiBellman",
                                     "The entry \"Discretization\" should be "
                                     "in format \"x_min delta_x Nx\" for "
                                     "every dimension.");
        Ndimension_ = discretization_vector.size() / 3;
        x_min_.Reallocate(Ndimension_);
        Delta_x_.Reallocate(Ndimension_);
        Nx_.Reallocate(Ndimension_);
        Npoint_ = 1;
        for (int i = 0; i < Ndimension_; i++)
        {
            to_num(discretization_vector[3 * i], x_min_(i));
            to_num(discretization_vector[3 * i + 1], Delta_x_(i));
            to_num(discretization_vector[3 * i + 2], Nx_(i));
            Npoint_ *= Nx_(i);
        }

        // Checks consistency of 'Ndimension_' with the model state.
        if (Ndimension_ != model_.GetNstate())
            throw ErrorConfiguration("HamiltonJacobiBellman::"
                                     "HamiltonJacobiBellman",
                                     "The dimension of the model ("
                                     + to_str(model_.GetNstate())
                                     + ") is incompatible with that of "
                                     " the HJB solver (" + to_str(Ndimension_)
                                     + ").");

        configuration_stream.set("Initial_date", initial_date_);
        configuration_stream.set("Delta_t", Delta_t_);
        configuration_stream.set("Nt", Nt_);

        configuration_stream.set("Model_time_dependent",
                                 model_time_dependent_);

        /*** Equation coefficients ***/

        configuration_stream.set_prefix("HJB/equation_coefficients/");

        string Q_0;
        configuration_stream.set("Q_0", Q_0);
        vector<string> Q_0_vector = split(Q_0);
        if (int(Q_0_vector.size()) != Ndimension_ * Ndimension_)
            throw ErrorConfiguration("HamiltonJacobiBellman::"
                                     "HamiltonJacobiBellman",
                                     "The entry \"Q_0\" should be "
                                     "a matrix with "
                                     + to_str(Ndimension_ * Ndimension_)
                                     + " elements, but "
                                     + to_str(Q_0_vector.size()) + " elements"
                                     " were provided.");
        Q_0_.Reallocate(Ndimension_, Ndimension_);
        for (int i = 0; i <  Ndimension_; i++)
            for (int j = 0; j <  Ndimension_; j++)
                to_num(Q_0_vector[i * Ndimension_ + j], Q_0_(i, j));

        /*** Solver ***/

        configuration_stream.set_prefix("HJB/solver/");

        configuration_stream.set("Scheme", scheme_,
                                 "'LxF' | 'BrysonLevy' | 'Godunov'");

        /*** Boundary condition ***/

        configuration_stream.set_prefix("HJB/boundary_condition/");

        configuration_stream.set("Type", boundary_condition_type_,
                                 "'Dirichlet' | 'Extrapolation' "
                                 "| 'Periodic'");
        if (boundary_condition_type_ == "Dirichlet")
            boundary_condition_index_ = 0;
        else if (boundary_condition_type_ == "Extrapolation")
            boundary_condition_index_ = 1;
        else
            boundary_condition_index_ = 2;
        configuration_stream.set("Value", boundary_condition_, ">= 0");

        /*** Lax-Friedrichs scheme ***/

        if (scheme_ == "LxF")
        {
            configuration_stream.set_prefix("HJB/lax_friedrichs/");

            string upper_bound_model;
            configuration_stream.set("Upper_bound_model", upper_bound_model);
            vector<string> bound_vector = split(upper_bound_model);
            if (int(bound_vector.size()) != Ndimension_)
                throw ErrorConfiguration("HamiltonJacobiBellman::"
                                         "HamiltonJacobiBellman",
                                         "The entry \"Upper_bound_model\" "
                                         "should contain "
                                         + to_str(Ndimension_)
                                         + " elements, but "
                                         + to_str(bound_vector.size())
                                         + " elements were provided.");
            upper_bound_model_.Reallocate(Ndimension_);
            for (int i = 0; i < Ndimension_; i++)
                to_num(bound_vector[i], upper_bound_model_(i));
        }

        /*** Ouput saver ***/

        output_saver_.Initialize(configuration_file, "HJB/output_saver/");
        output_saver_.Empty("value_function");
    }


    //! Destructor.
    template <class T, class ClassModel, class ClassObservationManager>
    HamiltonJacobiBellman<T, ClassModel, ClassObservationManager>
    ::~HamiltonJacobiBellman()
    {
    }


    /////////////
    // METHODS //
    /////////////


    //! Initializes the solver.
    /*! Initializes the model and the observation manager. Optionally computes
      the analysis of the first step.
      \param[in] configuration_file configuration file.
    */
    template <class T, class ClassModel, class ClassObservationManager>
    void HamiltonJacobiBellman<T, ClassModel, ClassObservationManager>
    ::Initialize(string configuration_file)
    {
        MessageHandler::Send(*this, "all", "::Initialize begin");

        if (option_display_["show_date"])
            Logger::StdOut(*this, "Date: "
                           + to_str(T(time_step_) * Delta_t_));
        else
            Logger::Log<-3>(*this, "Date: "
                            + to_str(T(time_step_) * Delta_t_));
        if (option_display_["show_iteration"])
            Logger::StdOut(*this, "Iteration " + to_str(time_step_) + " -> "
                           + to_str(time_step_ + 1));
        else
            Logger::Log<-3>(*this, "Iteration " + to_str(time_step_) + " -> "
                            + to_str(time_step_ + 1));

        /*** Initializations ***/

        model_.Initialize(configuration_file);
        // observation_manager_.Initialize(model_, configuration_file);

        /*** Initial value function ***/

        V_.Reallocate(Npoint_);

        // Initial value function: V(0, x) = <Q_0 x, x>.
        Vector<T> x(Ndimension_), Qx(Ndimension_);
        for (int i = 0; i < Npoint_; i++)
        {
            get_coordinate(i, x_min_, Delta_x_, Nx_, x);
            Mlt(Q_0_, x, Qx);
            V_(i) = DotProd(Qx, x);
        }

        /*** Model ***/

        Vector<T> Mx;
        Mx_.Reallocate(Npoint_, Ndimension_);
        if (!model_time_dependent_)
        {
            courant_number_ = 0.;
            double date, time_step;
            for (int i_cell = 0; i_cell < Npoint_; i_cell++)
            {
                get_coordinate(i_cell, x_min_, Delta_x_, Nx_, x);

                model_.SetDate(initial_date_);
                model_.SetState(x);
                model_.Forward();
                model_.GetState(Mx);
                date = model_.GetDate();
                time_step = date - initial_date_;

                Add(-1., x, Mx);
                for (int d = 0; d < Ndimension_; d++)
                {
                    Mx(d) *= Delta_t_ / (Delta_x_(d) * time_step);
                    courant_number_ = max(courant_number_, abs(Mx(d)));
                }

                SetRow(Mx, i_cell, Mx_);
            }
        }

        Logger::Log(*this, "Courant number: " + to_str(courant_number_));

        /*** Evolution points ***/

        if (scheme_ == "BrysonLevy")
        {
            // Location of the evolution points: (a, a, a, ..., a).
            T a = 0;
            for (int d = 0; d < Ndimension_; d++)
                a += 1. / (Delta_x_(d) * Delta_x_(d));
            a = sqrt(a);
            for (int d = 0; d < Ndimension_; d++)
                a += 1. / Delta_x_(d);
            a = 1. / a;

            // 'a' over Delta_x.
            a_Delta_x_.Reallocate(Ndimension_);
            for (int d = 0; d < Ndimension_; d++)
                a_Delta_x_ = a / Delta_x_(d);
        }

        MessageHandler::Send(*this, "all", "initial value");

        MessageHandler::Send(*this, "all", "::Initialize end");
    }


    //! Initializes a step for the time integration of HJB equation.
    /*! Initializes a step for the model.
     */
    template <class T, class ClassModel, class ClassObservationManager>
    void HamiltonJacobiBellman<T, ClassModel, ClassObservationManager>
    ::InitializeStep()
    {
        MessageHandler::Send(*this, "all", "::InitializeStep begin");

        if (option_display_["show_date"])
            Logger::StdOut(*this, "Date: "
                           + to_str(T(time_step_) * Delta_t_));
        else
            Logger::Log<-3>(*this, "Date: "
                            + to_str(T(time_step_) * Delta_t_));
        if (option_display_["show_iteration"])
            Logger::StdOut(*this, "Iteration " + to_str(time_step_) + " -> "
                           + to_str(time_step_ + 1));
        else
            Logger::Log<-3>(*this, "Iteration " + to_str(time_step_) + " -> "
                            + to_str(time_step_ + 1));

        model_.InitializeStep();

        MessageHandler::Send(*this, "all", "::InitializeStep end");
    }


    //! Performs a step forward.
    template <class T, class ClassModel, class ClassObservationManager>
    void HamiltonJacobiBellman<T, ClassModel, ClassObservationManager>
    ::Forward()
    {
        MessageHandler::Send(*this, "all", "::Forward begin");

        if (scheme_ == "LxF")
            AdvectionLxFForward();
        else if (scheme_ == "BrysonLevy")
            AdvectionBrysonLevyForward();
        else
            AdvectionGodunov();

        MessageHandler::Send(*this, "all", "forecast value");

        MessageHandler::Send(*this, "all", "::Forward end");
    }


    //! Performs a step forward, using a first-order Lax-Friedrichs scheme.
    template <class T, class ClassModel, class ClassObservationManager>
    void HamiltonJacobiBellman<T, ClassModel, ClassObservationManager>
    ::AdvectionLxFForward()
    {
        MessageHandler::Send(*this, "all", "::AdvectionLxFForward begin");

        // Current value of V.
        Vector<T> V_cur = V_;

        // Position.
        Vector<int> position, position_left, position_right, position_cell;
        // Coordinates.
        Vector<T> x;
        // M(x).
        Vector<T> Mx;

        int i_left, i_right, i_cell;

        /*** Advection term ***/

        Vector<T> time_length_upper_bound(Ndimension_);
        for (int d = 0; d < Ndimension_; d++)
            time_length_upper_bound(d) = Delta_t_ / Delta_x_(d)
                * upper_bound_model_(d);

        // Model dates.
        double initial_date = initial_date_ + T(time_step_) * Delta_t_;
        double date, time_step;

        T time_delta = 0.;
        while (time_delta != Delta_t_)
        {
            if (model_time_dependent_)
            {
                courant_number_ = 0.;
                for (int i_cell = 0; i_cell < Npoint_; i_cell++)
                {
                    get_coordinate(i_cell, x_min_, Delta_x_, Nx_, x);

                    model_.SetDate(initial_date);
                    model_.SetState(x);
                    model_.Forward();
                    model_.GetState(Mx);
                    date = model_.GetDate();
                    time_step = date - initial_date;

                    Add(-1., x, Mx);
                    for (int d = 0; d < Ndimension_; d++)
                    {
                        Mx(d) *= Delta_t_ / (Delta_x_(d) * time_step);
                        courant_number_ = max(courant_number_, abs(Mx(d)));
                    }

                    SetRow(Mx, i_cell, Mx_);
                }
            }

            for (int i_cell = 0; i_cell < Npoint_; i_cell++)
            {
                get_position(i_cell, Nx_, position);

                GetRow(Mx_, i_cell, Mx);

                for (int d = 0; d < Ndimension_; d++)
                {
                    if (position(d) == Nx_(d) - 1)
                    {
                        position_left = position;
                        position_left(d)--;
                        i_left = get_position(Nx_, position_left);
                        if (boundary_condition_index_ == 1)
                            boundary_condition_ = 2. * V_cur(i_cell)
                                - V_cur(i_left);
                        else if (boundary_condition_index_ == 2)
                        {
                            position_right = position;
                            position_right(d) = 0;
                            i_right = get_position(Nx_, position_right);
                            boundary_condition_ = V_cur(i_right);
                        }
                        V_(i_cell) += -Mx(d)
                            * .5 * (boundary_condition_ - V_cur(i_left))
                            + time_length_upper_bound(d)
                            * (boundary_condition_ + V_cur(i_left)
                               - 2. * V_cur(i_cell));
                    }
                    else if (position(d) == 0)
                    {
                        position_right = position;
                        position_right(d)++;
                        i_right = get_position(Nx_, position_right);
                        if (boundary_condition_index_ == 1)
                            boundary_condition_ = 2. * V_cur(i_cell)
                                - V_cur(i_right);
                        else if (boundary_condition_index_ == 2)
                        {
                            position_left = position;
                            position_left(d) = Nx_(d) - 1;
                            i_left = get_position(Nx_, position_left);
                            boundary_condition_ = V_cur(i_left);
                        }
                        V_(i_cell) += -Mx(d)
                            * .5 * (V_cur(i_right) - boundary_condition_)
                            + time_length_upper_bound(d)
                            * (V_cur(i_right) + boundary_condition_
                               - 2. * V_cur(i_cell));
                    }
                    else
                    {
                        position_left = position;
                        position_left(d)--;
                        i_left = get_position(Nx_, position_left);
                        position_right = position;
                        position_right(d)++;
                        i_right = get_position(Nx_, position_right);
                        V_(i_cell) += -Mx(d)
                            * .5 * (V_cur(i_right) - V_cur(i_left))
                            + time_length_upper_bound(d)
                            * (V_cur(i_right) + V_cur(i_left)
                               - 2. * V_cur(i_cell));
                    }
                }
            }

            T limit = 0.5;
            if (courant_number_ > limit)
            {
                Logger::Log(*this, "Courant number: "
                            + to_str(courant_number_));
                T division = int(courant_number_ / limit) + 1;
                T local_step = Delta_t_ / division;
                // Checks that Delta_t_ is not reached before.
                if (time_delta + local_step > Delta_t_)
                {
                    local_step = Delta_t_ - time_delta;
                    time_delta = Delta_t_;
                }
                else
                    time_delta += local_step;
                local_step /= Delta_t_;
                for (int i = 0; i < Npoint_; i++)
                    V_(i) = V_cur(i) + (V_(i) - V_cur(i)) * local_step;
                Logger::Log(*this, "Local time step: " + to_str(local_step));
            }
            else
                time_delta = Delta_t_;
        }

        time_step_++;

        MessageHandler::Send(*this, "all", "::AdvectionLxFForward end");
    }


    /*! \brief Performs a step forward, using a first-order central scheme
      introduced in Bryson and Levy (SIAM J. Sci. Comput., 2003).
    */
    template <class T, class ClassModel, class ClassObservationManager>
    void HamiltonJacobiBellman<T, ClassModel, ClassObservationManager>
    ::AdvectionBrysonLevyForward()
    {
        MessageHandler::Send(*this, "all",
                             "::AdvectionBrysonLevyForward begin");

        // Direction derivatives of V.
        Matrix<T> V_x_m(Npoint_, Ndimension_), V_x_p(Npoint_, Ndimension_);

        // Position.
        Vector<int> position, position_left, position_right, position_cell;
        // Coordinates.
        Vector<T> x;
        // M(x).
        Vector<T> Mx;

        int i_left, i_right, i_cell;

        /*** Advection term ***/

        // Model dates.
        double initial_date = initial_date_ + T(time_step_) * Delta_t_;
        double date, time_step;

        if (model_time_dependent_)
        {
            courant_number_ = 0.;
            for (int i_cell = 0; i_cell < Npoint_; i_cell++)
            {
                get_coordinate(i_cell, x_min_, Delta_x_, Nx_, x);

                model_.SetDate(initial_date);
                model_.SetState(x);
                model_.Forward();
                model_.GetState(Mx);
                date = model_.GetDate();
                time_step = date - initial_date;

                Add(-1., x, Mx);
                for (int d = 0; d < Ndimension_; d++)
                {
                    Mx(d) *= Delta_t_ / (Delta_x_(d) * time_step);
                    courant_number_ = max(courant_number_, abs(Mx(d)));
                }

                SetRow(Mx, i_cell, Mx_);
            }
        }

        /*** Computing the directional derivatives of V ***/

        for (int i_cell = 0; i_cell < Npoint_; i_cell++)
        {
            get_position(i_cell, Nx_, position);

            for (int d = 0; d < Ndimension_; d++)
            {
                if (position(d) == Nx_(d) - 1)
                {
                    position_left = position;
                    position_left(d)--;
                    i_left = get_position(Nx_, position_left);
                    if (boundary_condition_index_ == 1)
                        boundary_condition_ = 2. * V_(i_cell) - V_(i_left);
                    else if (boundary_condition_index_ == 2)
                    {
                        position_right = position;
                        position_right(d) = 0;
                        i_right = get_position(Nx_, position_right);
                        boundary_condition_ = V_(i_right);
                    }
                    V_x_m(i_cell, d) = V_(i_cell) - V_(i_left);
                    V_x_p(i_cell, d) = boundary_condition_ - V_(i_cell);
                }
                else if (position(d) == 0)
                {
                    position_right = position;
                    position_right(d)++;
                    i_right = get_position(Nx_, position_right);
                    if (boundary_condition_index_ == 1)
                        boundary_condition_ = 2. * V_(i_cell) - V_(i_right);
                    else if (boundary_condition_index_ == 2)
                    {
                        position_left = position;
                        position_left(d) = Nx_(d) - 1;
                        i_left = get_position(Nx_, position_left);
                        boundary_condition_ = V_(i_left);
                    }
                    V_x_m(i_cell, d) = V_(i_cell) - boundary_condition_;
                    V_x_p(i_cell, d) = V_(i_right) - V_(i_cell);
                }
                else
                {
                    position_left = position;
                    position_left(d)--;
                    i_left = get_position(Nx_, position_left);
                    position_right = position;
                    position_right(d)++;
                    i_right = get_position(Nx_, position_right);
                    V_x_m(i_cell, d) = V_(i_cell) - V_(i_left);
                    V_x_p(i_cell, d) = V_(i_right) - V_(i_cell);
                }
            }
        }

        /*** Evolving the central values ***/

        int i_prev, i_next;
        for (int i_cell = 0; i_cell < Npoint_; i_cell++)
            for (int d = 0; d < Ndimension_; d++)
                V_(i_cell) += .25 * a_Delta_x_(d)
                    * (V_x_p(i_cell, d) - V_x_m(i_cell, d))
                    - .5 * Mx_(i_cell, d)
                    * (V_x_m(i_cell, d) + V_x_p(i_cell, d));

        time_step_++;

        MessageHandler::Send(*this, "all",
                             "::AdvectionBrysonLevyForward end");
    }


    //! Performs a step forward, using a first-order Godunov scheme.
    template <class T, class ClassModel, class ClassObservationManager>
    void HamiltonJacobiBellman<T, ClassModel, ClassObservationManager>
    ::AdvectionGodunov()
    {
        MessageHandler::Send(*this, "all", "::AdvectionGodunov begin");

        // Current value of V.
        Vector<T> V_cur = V_;

        // Position.
        Vector<int> position, position_left, position_right, position_cell;
        // Coordinates.
        Vector<T> x;
        // M(x).
        Vector<T> Mx;

        int i_left, i_right, i_cell;

        /*** Advection term ***/

        // Model dates.
        double initial_date = initial_date_ + T(time_step_) * Delta_t_;
        double date, time_step;

        T time_delta = 0.;
        if (model_time_dependent_)
        {
            courant_number_ = 0.;
            for (int i_cell = 0; i_cell < Npoint_; i_cell++)
            {
                get_coordinate(i_cell, x_min_, Delta_x_, Nx_, x);

                model_.SetDate(initial_date);
                model_.SetState(x);
                model_.Forward();
                model_.GetState(Mx);
                date = model_.GetDate();
                time_step = date - initial_date;

                Add(-1., x, Mx);
                for (int d = 0; d < Ndimension_; d++)
                {
                    Mx(d) *= Delta_t_ / (Delta_x_(d) * time_step);
                    courant_number_ = max(courant_number_, abs(Mx(d)));
                }

                SetRow(Mx, i_cell, Mx_);
            }
        }

        for (int i_cell = 0; i_cell < Npoint_; i_cell++)
        {
            get_position(i_cell, Nx_, position);

            GetRow(Mx_, i_cell, Mx);

            for (int d = 0; d < Ndimension_; d++)
            {
                if (position(d) == Nx_(d) - 1)
                {
                    position_left = position;
                    position_left(d)--;
                    i_left = get_position(Nx_, position_left);
                    if (boundary_condition_index_ == 1)
                        boundary_condition_ = 2. * V_cur(i_cell)
                            - V_cur(i_left);
                    else if (boundary_condition_index_ == 2)
                    {
                        position_right = position;
                        position_right(d) = 0;
                        i_right = get_position(Nx_, position_right);
                        boundary_condition_ = V_cur(i_right);
                    }
                    if (Mx(d) < 0.)
                        V_(i_cell) -=
                            Mx(d) * (boundary_condition_ - V_cur(i_cell));
                    else
                        V_(i_cell) -=
                            Mx(d) * (V_cur(i_cell) - V_cur(i_left));
                }
                else if (position(d) == 0)
                {
                    position_right = position;
                    position_right(d)++;
                    i_right = get_position(Nx_, position_right);
                    if (boundary_condition_index_ == 1)
                        boundary_condition_ = 2. * V_cur(i_cell)
                            - V_cur(i_right);
                    else if (boundary_condition_index_ == 2)
                    {
                        position_left = position;
                        position_left(d) = Nx_(d) - 1;
                        i_left = get_position(Nx_, position_left);
                        boundary_condition_ = V_cur(i_left);
                    }
                    if (Mx(d) < 0.)
                        V_(i_cell) -=
                            Mx(d) * (V_cur(i_right) - V_cur(i_cell));
                    else
                        V_(i_cell) -=
                            Mx(d) * (V_cur(i_cell) - boundary_condition_);
                }
                else
                {
                    position_left = position;
                    position_left(d)--;
                    i_left = get_position(Nx_, position_left);
                    position_right = position;
                    position_right(d)++;
                    i_right = get_position(Nx_, position_right);
                    if (Mx(d) < 0.)
                        V_(i_cell) -=
                            Mx(d) * (V_cur(i_right) - V_cur(i_cell));
                    else
                        V_(i_cell) -=
                            Mx(d) * (V_cur(i_cell) - V_cur(i_left));
                }
            }
        }

        time_step_++;

        MessageHandler::Send(*this, "all", "::AdvectionGodunov end");
    }


    //! Checks whether the model has finished.
    /*!
      \return True if no more data assimilation is required, false otherwise.
    */
    template <class T, class ClassModel, class ClassObservationManager>
    bool HamiltonJacobiBellman<T, ClassModel, ClassObservationManager>
    ::HasFinished() const
    {
        return time_step_ == Nt_;
    }


    //! Returns the model.
    /*!
      \return The model.
    */
    template <class T, class ClassModel, class ClassObservationManager>
    const ClassModel&
    HamiltonJacobiBellman<T, ClassModel, ClassObservationManager>
    ::GetModel() const
    {
        return model_;
    }


    //! Returns the name of the class.
    /*!
      \return The name of the class.
    */
    template <class T, class ClassModel, class ClassObservationManager>
    string HamiltonJacobiBellman<T, ClassModel, ClassObservationManager>
    ::GetName() const
    {
        return "HamiltonJacobiBellman";
    }


    //! Receives and handles a message.
    /*
      \param[in] message the received message.
    */
    template <class T, class ClassModel, class ClassObservationManager>
    void HamiltonJacobiBellman<T, ClassModel, ClassObservationManager>
    ::Message(string message)
    {
        if (message.find("initial value") != string::npos
            || message.find("forecast value") != string::npos)
            output_saver_.Save(V_, time_step_, "value_function");
    }


} // namespace Verdandi.


#define VERDANDI_FILE_HAMILTONJACOBIBELLMAN_CXX
#endif
