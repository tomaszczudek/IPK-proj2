#include "params.hpp"
#include "type.hpp"

int main(int argc, char **argv)
{
    Params params;

    try
    {
        params.parseArgs(argc, argv);

        #if 0
        if (params.isServer())
        {
            Server server(params);
        }
        else
        {
            Client client(params);
        }
        #endif
    }
    catch (int error)
    {
        return error;
    }

    return EX_OK;
}