// Copyright (C) 2009-2010 INRIA
// Author(s): Vivien Mallet
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


#ifndef VERDANDI_FILE_MODEL_QUADRATICMODEL_HXX


namespace Verdandi
{


    /////////////////////
    // QUADRATIC MODEL //
    /////////////////////


    //! This class is a quadratic model.
    /*! The model is defined as \f$\frac{\mathrm{d}x_i}{\mathrm{d}t} = x^T Q_i
      x + L_i x + b_i\f$, where \f$Q_i\f$ is a matrix, \f$L_i\f$ is the
      \f$i\f$-th row of the matrix \f$L\f$ and \f$b\f$ a vector.
      \tparam T the type of floating-point numbers.
    */
    template <class T>
    class QuadraticModel: public VerdandiBase
    {
    public:
        typedef T value_type;
        typedef T* pointer;
        typedef const T* const_pointer;
        typedef T& reference;
        typedef const T& const_reference;
        typedef Matrix<T> background_error_variance;
        typedef Vector<T> error_covariance_row;
        typedef Vector<T> state_vector;
        typedef Matrix<T> crossed_matrix;

    protected:

        //! Dimension of the state.
        int Nstate_;

        //! State vector.
        Vector<T> state_;

        //! Should the quadratic term be applied?
        bool with_quadratic_term_;
        //! Should the linear term be applied?
        bool with_linear_term_;
        //! Should the constant term be applied?
        bool with_constant_term_;

        //! Quadratic terms.
        vector<Matrix<T> > Q_;

        //! Matrix that defines the linear part of the model.
        Matrix<T> L_;

        //! Vector that defines the constant part of the model.
        Vector<T> b_;

        //! Time step.
        double Delta_t_;

        //! Final date of the simulation.
        double final_date_;

        //! Current date.
        double date_;

        //! Temporary variable that stores Q times the state vector.
        Vector<T> Q_state_;

        /*** Output saver ***/

        //! Output saver.
        OutputSaver output_saver_;

    public:
        // Constructors and destructor.
        QuadraticModel();
        QuadraticModel(string configuration_file);
        ~QuadraticModel();
        // Initializations.
        void Initialize(string configuration_file);
        void InitializeStep();

        // Processing.
        void Forward();
        bool HasFinished() const;
        void Save();

        // Access methods.
        T GetDelta_t() const;
        double GetDate() const;
        void SetDate(double date);
        int GetNstate() const;
        void GetState(state_vector& state) const;
        void SetState(state_vector& state);
        void GetFullState(state_vector& state) const;
        void SetFullState(const state_vector& state);

        string GetName() const;
        void Message(string message);
    };


} // namespace Verdandi.


#define VERDANDI_FILE_MODEL_QUADRATICMODEL_HXX
#endif