/* mxSlimCurve.c mex Wrapper for EcfSingle function from SLIM-CURVE */

// Headers to include
#include "mex.h"
#include "Ecf.h"
#include <stdio.h>
#include <stdlib.h>

// Definitions
#define printf mexPrintf
#define __transient_values  prhs[0]  // double: transient(s)
#define __prompt_values     prhs[1]  // double: prompt(s)
#define __x_inc             prhs[2]  // time bin size
#define __fit_start         prhs[3]  // starting bin of the fit
#define __fit_type          prhs[4]  // fit type (default = 1)
#define __noise_model       prhs[5]  // noise model (default = 4)
#define __chi_sq_target     prhs[6]  // chi square target (default = 1.1)
#define __chi_sq_delta      prhs[7]  // chi square delta (default = 0.01)
#define __fit_end           prhs[8]  // end coordinate of the fit
                                     // (default = transient_size - 1)
#define __sigma_values      prhs[9]  // noise standard deviations
                                     // (default = [])

#define __LMA_param         plhs[0]  // LMA fit parameters
#define __RLD_param         plhs[1]  // RLD fit parameters
#define __LMA_fit           plhs[2]  // LMA fit result
#define __RLD_fit           plhs[3]  // RLD fit result

void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[])
{
    int i;  // General counting index

    /**************************/
    /* Parse mandatory inputs */
    /**************************/

    // Check if at least transient_values, prompt_values, x_inc and
    // fit_start are supplied
    if (nrhs < 4)
    {
        mexPrintf("You must specify at least transient_values, "
                  "prompt_values, x_inc and fit_start.\nTerminating.\n");
    }
    
    // transient_values:
    //      A double matrix of size MxN containing transients for a fit.
    //      The transients must be arranged in columns.
    int transient_size = mxGetM(__transient_values); 
    int transient_nr = mxGetN(__transient_values);
    // Check if the transient_size and transient_nr are positive and double
    // otherwise quit with a warning.
    if (transient_size < 2)
    {
        mexPrintf("transient_values must contain more than one point. "
                  "You chose transient with %d points.\nTerminating.\n",
                  transient_size);
        return;
    }
    if (transient_size > 2048)
    {
        mexPrintf("transient_values must not have more than 2048 points. "
                  "You chose transient with %d points.\nTerminating.\n",
                  transient_size);
        return;
    }
    if (transient_nr < 1)
    {
        mexPrintf("transient_values cannot be empty. You chose %d "
                  "number of transients.\nTerminating.\n", transient_nr);
        return;
    }
    if (!mxIsDouble(__transient_values))
    {
        mexPrintf("transient_values must be double. You given: %s\n"
                  "Terminating.\n", mxGetClassName(__transient_values));
        return;
    }
    float transient_values[transient_size];
    double *transient_ptr = mxGetPr(__transient_values);

    // prompt_values:
    //      A double matrix of size MxN containing prompts for a fit.
    //      There can be either one single prompt for all transients 
    //      arranged in Mx1 matrix or one prompt in an MxN matrix, where
    //      N, the number of columns, is the same as in the transient
    //      matrix.
    int prompt_size = mxGetM(__prompt_values);
    int prompt_nr = mxGetN(__prompt_values);
    if (prompt_size < 2)
    {
        mexPrintf("prompt_values must contain more than one point. "
                  "You chose transient with %d points.\nTerminating\n",
                  prompt_size);
        return;
    }
    if ((prompt_nr != 1) && (prompt_nr != transient_nr))
    {
        mexPrintf("Number of prompts must be either one or the same as "
                  "the number of transients. You specified: "
                  "%d prompts.\nTerminating.\n", prompt_nr);
        return;
    }
    if (!mxIsDouble(__prompt_values))
    {
        mexPrintf("prompt_values must be double. You given: %s\n"
                  "Terminating.\n", mxGetClassName(__prompt_values));
        return;
    }
    float prompt_values[prompt_size];
    double *prompt_ptr = mxGetPr(__prompt_values);
    for (i = 0; i < prompt_size; i++)
    {
        prompt_values[i] = prompt_ptr[i];
    }

    // x_inc:
    //      Float (single) with the time histogram bin size. It can be
    //      either a single value or a vector with number of points equal
    //      to the number of transients
    // Check if x_inc is a scalar or a vector
    int x_inc_nr = mxGetNumberOfElements(__x_inc);
    float x_inc;
    // check if x_inc number of elements is 1 or equal to transient_nr
    if ((x_inc_nr != 1) && (x_inc_nr != transient_nr))
    {
        mexPrintf("Number of elements in x_inc must be one or equal "
                  "to the number of transients. Your x_inc has got "
                  "%d elements.\nTerminating.\n", x_inc_nr);
        return;
    }
    // Store x_inc a double pointer array
    // check if x_inc is double, otherwise quit with warning
    if (!mxIsDouble(__transient_values))
    {
        mexPrintf("x_inc must be a double. You given: %s\n"
                  "Terminating.\n", mxGetClassName(__x_inc));
        return;
    }
    double *x_inc_ptr = mxGetPr(__x_inc);
    x_inc = (float) x_inc_ptr[0];

    // fit_start:
    //      Integer coordinate of the first point in the transient
    //      considered in the fit.
    //      Values ranging from 0 to the number of point in a transient
    //      minus two.
    if (mxGetNumberOfElements(__fit_start) != 1)
    {
        mexPrintf("fit_start must be a scalar. Your fit_start has got %d "
                  "elements.\nTerminating.\n",
                  mxGetNumberOfElements(__fit_start));
        return;
    }
    int fit_start = (int) mxGetScalar(__fit_start);
    // check if fit_start is between 0 and transient_size - 2,
    // otherwise quit with a warning.
    if ((fit_start < 0) | (fit_start > transient_size - 2))
    {
        mexPrintf("fit_start must be between 0 and %d. You chose: "
                  "fit_start = %d\nTerminating.\n", 
                  transient_size - 2, fit_start);
        return;
    }


    /*************************/
    /* Parse optional inputs */
    /*************************/

    // fit_type:
    //      Integer with values from 1 to 4
    //      1: Single exponential
    //      2: Double exponential
    //      3: Triple exponential
    //      4: Stretched exponential (fitting function GCI_stretchedexp)
    int fit_type = 1;
    if (nrhs > 4)
    {
        if (mxGetNumberOfElements(__fit_type) != 1)
        {
            mexPrintf("fit_type must be a scalar. Your fit_type "
                      "has got %d elements.\nTerminating.\n", 
                      mxGetNumberOfElements(__fit_type));
            return;
        }
        fit_type = (int) mxGetScalar(__fit_type);
        // check if fit_type is between 1 and 4, otherwise quit with a
        // warning
        if ((fit_type < 1) | (fit_type > 4))
        {
            mexPrintf("fit_type must be between 1 and 4. You chose: "
                      "fit_type = %d\nTerminating.\n", fit_type);
            return;
        }
    }

    // noise_model:
    //      Integer with values from 1 to 6
    //      0: NOISE_CONST
    //      1: NOISE_GIVEN
    //      2: NOISE_POISSON_DATA
    //      3: NOISE_POISSON_FIT
    //      4: NOISE_GAUSSIAN_FIT
	//      5: NOISE_MLE
    int noise_model = 4;
    if (nrhs > 5)
    {
        if (mxGetNumberOfElements(__noise_model) != 1)
        {
            mexPrintf("noise_model must be a scalar. Your noise_model "
                      "has got %d elements.\nTerminating.\n", 
                      mxGetNumberOfElements(__noise_model));
            return;
        }
        noise_model = (int) mxGetScalar(__noise_model);
        // check if noise_model is between 1 and 6, otherwise quit with a
        // warning
        if ((noise_model < 1) | (noise_model > 6))
        {
            mexPrintf("noise_model must be between 1 and 6. You chose: "
                      "noise_model = %d\nTerminating.\n", noise_model);
            return;
        }
    }

    // chi_sq_target:
    //      Float (single) larger than 1 but close to 1.
    //      Recommended value 1.1
    //      Larger values result in faster albeit less optimized fits.
    float chi_sq_target = 1.1;
    if (nrhs > 6)
    {
        if (mxGetNumberOfElements(__chi_sq_target) != 1)
        {
            mexPrintf("chi_sq_target must be a scalar. Your chi_sq_target "
                      "has got %d elements.\nTerminating.\n", 
                      mxGetNumberOfElements(__chi_sq_target));
            return;
        }
        chi_sq_target = (float) mxGetScalar(__chi_sq_target);
        // check if chi_sq_target is >= 1, otherwise quit with a warning
        if ((chi_sq_target < 1))
        {
            mexPrintf("chi_sq_target must be greater than 1. You chose: "
                      "chi_sq_target = %g\nTerminating.\n", chi_sq_target);
            return;
        }
    }

    // chi_sq_delta:
    //      Float (single) larger than 0 but close to 0 and less than 0.5.
    //      Iterations of fitting continue until the change in chi square 
    //      is below this value. 1e-6 is a "very strict" value.
    //      Recommended value 0.01
    float chi_sq_delta = 0.001;
    if (nrhs > 7)
    {
        if (mxGetNumberOfElements(__chi_sq_delta) != 1)
        {
            mexPrintf("chi_sq_delta must be a scalar. Your chi_sq_delta "
                      "has got %d elements.\nTerminating.\n", 
                      mxGetNumberOfElements(__chi_sq_delta));
            return;
        }
        chi_sq_delta = (float) mxGetScalar(__chi_sq_delta);
        // check if chi_sq_delta is non-negative, otherwise quit with a 
        // warning
        if ((chi_sq_delta < 0) || (chi_sq_delta >= 0.5))
        {
            mexPrintf("chi_sq_delta must be greater than 0 and less "
                      "than 0.5. You chose: chi_sq_delta = %g\n"
                      "Terminating.\n", chi_sq_delta);
            return;
        }
    }

    // fit_end:
    //      Integer coordinate of the last point in the transient
    //      considered in the fit.
    //      Values ranging from fit_start to the number of point in a
    //      transient minus one.
    int fit_end = transient_size - 1;
    if (nrhs > 8)
    {
        if (mxGetNumberOfElements(__fit_end) != 1)
        {
            mexPrintf("fit_end must be a scalar. Your fit_end has got %d "
                      "elements.\nTerminating.\n", 
                      mxGetNumberOfElements(__fit_end));
            return;
        }
        fit_end = (int) mxGetScalar(__fit_end);
        // check if fit_end is between fit_start+1 and transient_size-1,
        // otherwise quit with a warning.
        if ((fit_end <= fit_start) | (fit_end >= transient_size))
        {
            mexPrintf("fit_end must be between %d and %d. You chose: "
                      "fit_end = %d\nTerminating.\n", fit_start + 1, 
                      transient_size - 1, fit_end);
            return;
        }
    }


    // sigma_values (optional):
    //      A double matrix of size MxN containing standard deviations
    //      for indvidual noise characteristics to each transient point
    //      with noise type "given".
    //      There can be either one single sigma for all transients 
    //      arranged in Mx1 matrix or one sigma in an MxN matrix, where
    //      N, the number of columns, is the same as in the transient
    //      matrix.
    int sigma_size = 0;
    int sigma_nr = 0;  
    if (nrhs > 9)
    {
        sigma_nr = mxGetN(__sigma_values);
        sigma_size = mxGetM(__sigma_values);
        // check if sigma_nr is 0, 1 or transient_nr,
        // otherwise quit with a warning.
        if ((sigma_nr != 0) && (sigma_nr !=1) && (sigma_nr !=transient_nr))
        {
            mexPrintf("Number of sigmas must be either zero, one or the "
                      "same as the number of transients. You specified: "
                      "%d sigmas.\nTerminating.\n", sigma_nr);
            return;
        }
        // check if sigma_size is 0 or transient_size,
        // otherwise quit with a warning.
        if ((sigma_size != 0) && (sigma_size != transient_size))
        {
            mexPrintf("Points in a sigma must be either zero or same as "
                      "the number points in a transient. You specified: "
                      "%d points in a sigma.\nTerminating.\n", sigma_size);
            return;
        }
        // check if sigma_values are double, otherwise quit with a warning.
        if (!mxIsDouble(__sigma_values))
        {
            mexPrintf("sigma_values must be double. You given %s\n"
                      "Terminating.\n", mxGetClassName(__sigma_values));
            return;
        }
    }
    float sigma_values[sigma_size];
    double *sigma_ptr;
    if (nrhs > 9)
    {
        double *sigma_ptr = mxGetPr(__sigma_values);
        for (i = 0; i < sigma_size; i++)
        {
            sigma_values[i] = (float) sigma_ptr[i];
        }
    }

    /**************************/
    /*      Perform fits      */
    /**************************/
    int fits;               // Counting index of the fit
    float a, tau, z;        // Return values for RLD fit
    // Pointer to an array holding fitted transient
    float *fitted = 
            (float *)malloc((unsigned)transient_size * sizeof(float));
    // Pointer to an array holding fitted transient residuals
    float *residuals =
            (float *)malloc((unsigned)transient_size * sizeof(float));
    float chi_square;   // Resulting chi_square value
    int chi_sq_adjust;  // Adjustment integer for chi_square
                        // (not sure about function)
    int n_param;        // Number of parameters for given model
    float *params;      // Pointer to array of fit parameters (not used)
    int *param_free;    // Array with switches for free/fixed parameters
                        // (not used)
    int n_param_free;   // Number of free parameters
    int restrain;       // Limits for fit parameters (not used)
    int chi_sq_percent; // (not sue about function)
    int n_data;         // number of points in a transient
    int return_value;   // return value from fitting functions

    // Fitting function for the noise model
    void (*fitfunc)(float, float [], float *, float[], int) = NULL;

    switch (fit_type) {
        // single exponential
        case 1:
            // params are Z, A, T
            n_param = 3;
            params = (float *)malloc((size_t) n_param * sizeof(float));
            break;
        // double exponential
        case 2:
            // params are Z, A1, T1, A2, T2
            n_param = 5;
            params = (float *)malloc((size_t) n_param * sizeof(float));
            break;
        // triple exponential
        case 3:
            // params are Z, A1, T1, A2, T2, A3, T3
            n_param = 7;
            params = (float *)malloc((size_t) n_param * sizeof(float));
            break;
        // stretched exponential
        case 4:
            // has it's own fitfunc
            fitfunc = GCI_stretchedexp;
            // params are Z, A, T, H
            n_param = 4;
            params = (float *)malloc((size_t) n_param * sizeof(float));
            break;
    }
    // param_free array describes which params are free vs fixed (omits X2)
    param_free = (int *)malloc((size_t) n_param  * sizeof(int));

    /* Create output pointers */
    // LMA fit parameters
    double *LMA_param_out;
    int LMA_param_in = 0;
    __LMA_param = mxCreateDoubleMatrix(n_param + 1, transient_nr, mxREAL);
    LMA_param_out = mxGetPr(__LMA_param);


    // RLD fit parameters
    double *RLD_param_out;
    int RLD_param_in = 0;
    if (nlhs > 1)
    {
        __RLD_param = mxCreateDoubleMatrix(3, transient_nr, mxREAL);
        RLD_param_out = mxGetPr(__RLD_param);
    }


    // LMA-fitted curve
    double *LMA_fitted_out;
    int LMA_fitted_in = 0;
    if (nlhs > 2)
    {
        __LMA_fit = 
                mxCreateDoubleMatrix(transient_size, transient_nr, mxREAL);
        LMA_fitted_out = mxGetPr(__LMA_fit);
    }

    // RLD-fitted curve
    double *RLD_fitted_out;
    int RLD_fitted_in = 0;
    if (nlhs > 3)
    {
        __RLD_fit = 
                mxCreateDoubleMatrix(transient_size, transient_nr, mxREAL);
        RLD_fitted_out = mxGetPr(__RLD_fit);
    }
    
    // Run a fitting loop for each transient
    for (fits = 0; fits < transient_nr; fits++)
    {
        // Feed data into "transient_values" array
        for (i = 0; i < transient_size; i++)
        {
            transient_values[i] = 
                    (float) transient_ptr[i + transient_size * fits];
        }
        if (fits > 0)
        {
            // If each transient comes with a different prompt ...
            if (prompt_nr > 1)
            {
                // ... feed new data into "prompt_values" array
                for (i = 0; i < prompt_size; i++)
                {
                    prompt_values[i] = 
                            (float) prompt_ptr[i + prompt_size * fits];
                }
            }
            // If each transient comes with a different sigma ...
            if (sigma_nr > 1)
            {
                // ... feed new data into "simga_values" array
                for (i = 0; i < sigma_size; i++)
                {
                    sigma_values[i] = 
                            (float) sigma_ptr[i + sigma_size * fits];
                }
            }
            // If each transient comes with a x_inc ...
            if (x_inc_nr > 1)
            {
                // ... update x_inc
                x_inc = (float) x_inc_ptr[fits];
            }
        }

        // Now all input arrays are ready to be send for fitting.

        restrain = 0;           // Reset to starting values
        chi_sq_percent = 95;    // Reset to starting values
        n_data = fit_end;       // Reset to starting values

        // I am not sure what is going on here.
        float **covar    = GCI_ecf_matrix(n_data, n_data);
        float **alpha    = GCI_ecf_matrix(n_data, n_data);
        float **err_axes = GCI_ecf_matrix(n_data, n_data);

        // blind initial estimates as in TRI2/SP
        a = 1000.0f;
        tau = 2.0f;
        z = 0.0f;

        // want non-reduced chi square target
        chi_sq_adjust = fit_end - fit_start - 3;

        // for RLD+LMA fits TRI2/SP adjusts as follows for initial RLD fit:
        //  fit_start becomes index of peak of transient
        //  estimated A becomes value at peak
        //  noise becomes NOISE_POISSON_FIT

        // Run RLD fitting routine
        return_value = GCI_triple_integral_fitting_engine(
                        x_inc,
                        transient_values,
                        fit_start,
                        fit_end,
                        prompt_values,
                        prompt_size,
                        noise_model,
                        sigma_values,
                        &z,
                        &a,
                        &tau,
                        fitted,
                        residuals,
                        &chi_square,
                        chi_sq_target * chi_sq_adjust
                        );

        // If the parameters of RLD fit are requested, fill up the output
        // array.
        if (nlhs > 1)
        {
            RLD_param_out[RLD_param_in] = (double) z; RLD_param_in++;
            RLD_param_out[RLD_param_in] = (double) a; RLD_param_in++;
            RLD_param_out[RLD_param_in] = (double) tau; RLD_param_in++;
        }

        // If the RLD fit is requested, fill up the output array.
        if (nlhs > 3)
        {
            for (i = 0; i < transient_size; i++)
            {
                RLD_fitted_out[RLD_fitted_in] = (double) fitted[i];
                RLD_fitted_in++;
            }
        }

        // adjust single exponential estimates for multiple exponential
        // fits.
        fitfunc = GCI_multiexp_tau;
        switch (fit_type) {
            // single exponential
            case 1:
                // params are Z, A, T
                params[0] = z;
                params[1] = a;
                params[2] = tau;
                break;
            // double exponential
            case 2:
                // params are Z, A1, T1, A2, T2
                params[0] = z;
                params[1] = 0.75f * a; // values from TRI2/SP
                params[2] = tau;
                params[3] = 0.25f * a;
                params[4] = 0.6666667f * tau;
                break;
            // triple exponential
            case 3:
                // params are Z, A1, T1, A2, T2, A3, T3
                params[0] = z;
                params[1] = 0.75f * a;
                params[2] = tau;
                params[3] = 0.1666667f * a;
                params[4] = 0.6666667f * tau;
                params[5] = 0.1666667f * a;
                params[6] = 0.3333333f * tau;
                break;
            // stretched exponential
            case 4:
                // has it's own fitfunc
                // fitfunc = GCI_stretchedexp;

                // params are Z, A, T, H
                params[0] = z;
                params[1] = a;
                params[2] = tau;
                params[3] = 1.5f;
                break;
        }

        // Make sure all parameters are free in the fit
        n_param_free = 0;
        for (i = 0; i < n_param; ++i) {
            param_free[i] = 1;
            ++n_param_free;
        }

        chi_sq_adjust = fit_end - fit_start - n_param_free;

        // Run LMA fitting routine
        return_value = GCI_marquardt_fitting_engine(
                                            x_inc,
                                            transient_values,
                                            transient_size,
                                            fit_start,
                                            fit_end,
                                            prompt_values,
                                            prompt_size,
                                            noise_model,
                                            sigma_values,
                                            params,
                                            param_free,
                                            n_param,
                                            restrain,
                                            fitfunc,
                                            fitted,
                                            residuals,
                                            &chi_square,
                                            covar,
                                            alpha,
                                            err_axes,
                                            chi_sq_target * chi_sq_adjust,
                                            chi_sq_delta,
                                            chi_sq_percent);

        // Fill up the ouput array with the fit parameters
        for (i = 0; i < n_param; i++)
        {
             LMA_param_out[LMA_param_in] = (double) params[i];
             LMA_param_in++;
        }
        LMA_param_out[LMA_param_in] = (double) chi_square / chi_sq_adjust;
        LMA_param_in++;
        
        // If requested by Matlab, fill up the output array with the fitted
        // transient values
        if (nlhs > 2)
        {
            for (i = 0; i < transient_size; i++)
            {
                LMA_fitted_out[LMA_fitted_in] = (double) fitted[i];
                LMA_fitted_in++;
            }
        }
            
        // Not sure what is going on here
        GCI_ecf_free_matrix(covar);
        GCI_ecf_free_matrix(alpha);
        GCI_ecf_free_matrix(err_axes);
    }

    // Free memory to prevent leaks
    free(params);
    free(fitted);
    free(residuals);

    // That's it!
    return;
}




