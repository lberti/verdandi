// Copyright (C) 2009 INRIA
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


#ifndef VERDANDI_FILE_METHOD_FORWARDDRIVER_HXX


namespace Verdandi
{


    ///////////////////
    // FORWARDDRIVER //
    ///////////////////


    //! This class simply performs a forward simulation.
    template <class ClassModel>
    class ForwardDriver: public VerdandiBase
    {

    public:
        //! Type of the model state vector.
        typedef typename ClassModel::state model_state;

    protected:

        //! Underlying model.
        ClassModel model_;

        //! Iteration.
        int iteration_;
        //! Time vector.
        Vector<double> time_;

        /*** Configuration ***/

        //! Should the iterations be displayed?
        bool show_iteration_;
        //! Should the current time be displayed?
        bool show_time_;

        /*** Output saver ***/

        //! Output saver.
        OutputSaver output_saver_;

    public:

        /*** Constructor and destructor ***/

        ForwardDriver(string configuration_file);
        ~ForwardDriver();

        /*** Methods ***/

        void Initialize(string configuration_file);
        void InitializeStep();
        void Forward();

        bool HasFinished() const;

        // Access methods.
        const ClassModel& GetModel() const;
        string GetName() const;
        void Message(string message);
    };


} // namespace Verdandi.


#define VERDANDI_FILE_METHOD_FORWARDDRIVER_HXX
#endif
