// Copyright (C) 2008-2009 INRIA
// Author(s): Dominique Chapelle, Philippe Moireau
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


#ifndef VERDANDI_FILE_MODEL_CLAMPEDBAR_HXX

#include "seldon/SeldonSolver.hxx"

#include "OutputSaver.cxx"


namespace Verdandi
{


    //////////////////////
    // CLAMPEDBAR MODEL //
    //////////////////////


    //! This class is a clamped-bar model.
    /*!
      \tparam T the type of floating-point numbers.
    */
    template <class T>
    class ClampedBar: public VerdandiBase
    {
    public:
        typedef T value_type;
        typedef T* pointer;
        typedef const T* const_pointer;
        typedef T& reference;
        typedef const T& const_reference;
#ifdef VERDANDI_SPARSE
        typedef Matrix<T, General, RowSparse> background_error_variance;
        typedef Matrix<T, General, RowSparse> crossed_matrix;
        typedef Matrix<T, General, RowSparse> tangent_operator_matrix;
#else
        typedef Matrix<T> background_error_variance;
        typedef Matrix<T> crossed_matrix;
        typedef Matrix<T> tangent_operator_matrix;
#endif
        typedef Vector<T> error_covariance_row;
        typedef Vector<T> state_vector;


    protected:
        //! Bar length.
        double bar_length_;
        //! Space step along x.
        double Delta_x_;
        //! Number of elements along x.
        int Nx_;
        //! Number of degrees of freedom (dofs).
        int Ndof_;

        //! Time step.
        double Delta_t_;
        //! Current date.
        double date_;
        //! Simulation duration.
        double final_date_;
        //! Simulation dates.
        vector<double> date_vector_;

        //! Mass parameter.
        double mass_density_;
        //! Young's Modulus.
        double Young_modulus_;

        //! FEM Vector.
        Vector<T> disp_0_;
        Vector<T> velo_0_;
        Vector<T> disp_1_;
        Vector<T> velo_1_;
        Vector<T> force_;

        //! Mass FEM matrix.
        Matrix<T, General, RowMajor> Mass_matrix_el_;
        //! Stiffness FEM matrix.
        Matrix<T, General, RowMajor> Stiff_matrix_el_;

        //! Newmark Global FEM matrix.
        Matrix<T, Symmetric, RowSymSparse> Mass_matrix_;
        Matrix<T, Symmetric, RowSymSparse> Newmark_matrix_0_;
        Matrix<T, Symmetric, RowSymSparse> Newmark_matrix_1_;

#if defined(VERDANDI_WITH_DIRECT_SOLVER)
#if defined(SELDON_WITH_UMFPACK)
        MatrixUmfPack<T> mat_lu;
#elif defined(SELDON_WITH_SUPERLU)
        MatrixSuperLU<T> mat_lu;
#elif defined(SELDON_WITH_MUMPS)
        MatrixMumps<T> mat_lu;
#endif
#endif

        //! Balgovind scale for background covariance.
        double Balgovind_scale_background_;
        //! Background error variance.
        double background_error_variance_value_;

        //! Background error covariance matrix (B).
        background_error_variance background_error_variance_;

        //! Number of the row of B currently stored.
        int current_row_;
        //! Number of the column of Q currently stored.
        int current_column_;
        //! Value of the row of B currently stored.
        error_covariance_row error_covariance_row_;

        /*** Output saver ***/

        //! Output saver.
        OutputSaver output_saver_;

    public:
        // Constructor and destructor.
        ClampedBar();
        ClampedBar(string configuration_file);
        ~ClampedBar();
        void Initialize(string configuration_file);
        void InitializeFirstStep();
        void InitializeStep();

        // Processing.
        void Forward();
        bool HasFinished() const;
        void Save();

        // Access methods.
        double GetDate() const;
        int GetNstate() const;
        void GetState(state_vector& state) const;
        void SetState(state_vector& state);
        void GetFullState(state_vector& state) const;
        void SetFullState(const state_vector& state);

        void GetBackgroundErrorCovarianceRow(int row,
                                             error_covariance_row&
                                             error_covariance_row);
        const background_error_variance&
        GetBackgroundErrorVarianceMatrix() const;
        bool IsErrorSparse() const;

        string GetName() const;
        void Message(string message);

    };


} // namespace Verdandi.


#define VERDANDI_FILE_MODEL_CLAMPEDBAR_HXX
#endif
