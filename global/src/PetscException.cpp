#include "PetscException.hpp"


//Positive codes mean that there's an error
//Zero means success
//Negative codes should never happen, but we'll throw anyway
void PetscException(PetscInt petscError,
                    unsigned line,
                    const char* funct,
                    const char* file)
{
    if (petscError != 0)
    {
        const char*  p_text;
        char default_message[30]="Unknown PETSc error code";
        
        //PetscErrorMessage will swing p_text to point to the error code's message
        //...but only if it's a valid code
        PetscErrorMessage(petscError,  &p_text, NULL);
        if (p_text == 0)
        {
            p_text=default_message;
        }
        
        std::string err_string;
        err_string=p_text;
        err_string+= " in function '";
        err_string+= funct;
        err_string+= "' on line " ;
        err_string+= line;
        err_string+= " of file ";
        err_string+= file;
        
        EXCEPTION(err_string);
    }
}

//Positive codes mean that the KSP converged
//Negative codes mean that the KSP diverged i.e. there's a problem
void KspException(PetscInt kspError,
                  unsigned line,
                  const char* funct,
                  const char* file)
{
    std::string err_string;
    if (kspError < 0)
    {
        switch (kspError)
        {
            case KSP_DIVERGED_ITS:
                err_string = "KSP_DIVERGED_ITS";
                
                break;
            case KSP_DIVERGED_DTOL:
                err_string = "KSP_DIVERGED_DTOL";
                
                break;
            case KSP_DIVERGED_BREAKDOWN:
                err_string = "KSP_DIVERGED_BREAKDOWN";
                
                break;
            case KSP_DIVERGED_BREAKDOWN_BICG:
                err_string = "KSP_DIVERGED_BREAKDOWN_BICG";
                
                break;
            case KSP_DIVERGED_NONSYMMETRIC:
                err_string = "KSP_DIVERGED_NONSYMMETRIC";
                
                break;
            case KSP_DIVERGED_INDEFINITE_PC:
                err_string = "KSP_DIVERGED_INDEFINITE_PC";
                
                break;
            default:
                err_string = "Unknown KSP error code";
        }
        
        err_string+= " in function '";
        err_string+= funct;
        err_string+= "' on line " ;
        err_string+= line;
        err_string+= " of file ";
        err_string+= file;
        
        EXCEPTION(err_string);
    }
}
