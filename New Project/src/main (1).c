#include "ui.h"
int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);
    Data *appData = g_malloc(sizeof(Data));
    appData->head1 = NULL;
    appData->head2 = NULL;
    appData->newNode1 = NULL;
    appData->newNode2 = NULL;
    appData->forwardCount = 0;
    appData->depthLevel = 0;
    appData->isCopyMode = 0;
    appData->isCutMode = 0;
    set_ui(appData);
}