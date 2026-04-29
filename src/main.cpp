#include "params.hpp"
#include "client.hpp"
#include "server.hpp"

int main(int argc, char **argv)
{
    Params params;

    try
    {
        params.parseArgs(argc, argv);

        if (params.getIsServer() == SERVER)
        {
            Server server(params);
            server.startServer();
        }
        else
        {
            Client client(params);
            client.startClient();
        }
    }
    catch (int error)
    {
        return error;
    }

    return EX_OK;
}