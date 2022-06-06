#include "TheGame.h"
#include "platform.h"
#include "utils.h"
#include "linmath.h"
#include "Texture.h"
#include "Shader.h"
#include "DrawJob.h"
#include "ModelBuilder.h"
#include "TexCoords.h"
#include "ModelLoader.h"
#include "model_car/CarWheel.h"

extern std::string filesRoot;
extern float degrees2radians;

std::vector<GameSubj*> TheGame::gameSubjs;

int TheGame::getReady() {
    bExitGame = false;
    Shader::loadShaders();
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);//default
    glFrontFace(GL_CCW);//default

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);

    int subjN = ModelLoader::loadModel(&gameSubjs, "/dt/models/cars/999_1935_deusenberg_ssj_speedster/root01.txt", "");
    GameSubj* pGS = gameSubjs.at(subjN);
    pGS->ownSpeed.eulerDg[1] = 1;

    //===== set up camera
    v3set(mainCamera.ownCoords.eulerDg, 25, 180, 0); //set camera angles/orientation
    Coords::eulerDgToMatrix(mainCamera.ownCoords.rotationMatrix, mainCamera.ownCoords.eulerDg);

    mainCamera.viewRangeDg = 20;
    mainCamera.stageSize[0] = 90;
    mainCamera.stageSize[1] = 60;
    memcpy(mainCamera.lookAtPoint, pGS->ownCoords.pos, sizeof(float) * 3);
    mainCamera.onScreenResize();

    //===== set up light
    v3set(dirToMainLight, -1, 1, 0.5);
    v3norm(dirToMainLight);

    return 1;
}
int TheGame::drawFrame() {
    myPollEvents(); 

    //glClearColor(0.0, 0.0, 0.5, 1.0);
    glDepthMask(GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mat4x4 mProjection, mViewProjection;
    float nearClip = mainCamera.focusDistance - 55;
    float farClip = mainCamera.focusDistance + 55;
    if (nearClip < 0) nearClip = 0;
    //mat4x4_ortho(mProjection, -(float)screenSize[0] / 2, (float)screenSize[0] / 2, -(float)screenSize[1] / 2, (float)screenSize[1] / 2, nearClip, farClip);
    mat4x4_perspective(mProjection, mainCamera.viewRangeDg * degrees2radians, screenAspectRatio, nearClip, farClip);
    mat4x4_mul(mViewProjection, mProjection, mainCamera.lookAtMatrix);
    mViewProjection[1][3] = 0; //keystone effect

    //scan subjects
    int subjsN = gameSubjs.size();
    for (int subjN = 0; subjN < subjsN; subjN++) {
        GameSubj* pGS = gameSubjs.at(subjN);
        //behavior - apply rotation speed
        pGS->moveSubj();
        //prepare subject for rendering
        pGS->buildModelMatrix(pGS);

        pGS->render(mViewProjection, &mainCamera, dirToMainLight);
    }
    //synchronization
    while (1) {
        long long int currentMillis = getSystemMillis();
        long long int millisSinceLastFrame = currentMillis - lastFrameMillis;
        if (millisSinceLastFrame >= millisPerFrame) {
            lastFrameMillis = currentMillis;
            break;
        }
    }
    mySwapBuffers();
    return 1;
}
int TheGame::cleanUp() {
    int itemsN = gameSubjs.size();
    //delete all UISubjs
    for (int i = 0; i < itemsN; i++)
        delete gameSubjs.at(i);
    gameSubjs.clear();
    //clear all other classes
    MaterialAdjust::cleanUp();
    Texture::cleanUp();
    Shader::cleanUp();
    DrawJob::cleanUp();
    MyColor::cleanUp();
    return 1;
}
int TheGame::onScreenResize(int width, int height) {
    if (screenSize[0] == width && screenSize[1] == height)
        return 0;
    screenSize[0] = width;
    screenSize[1] = height;
    screenAspectRatio = (float)width / height;
    glViewport(0, 0, width, height);
    mainCamera.onScreenResize();
    mylog(" screen size %d x %d\n", width, height);
    return 1;
}
int TheGame::run() {
    getReady();
    while (!bExitGame) {
        drawFrame();
    }
    cleanUp();
    return 1;
}
GameSubj* TheGame::newGameSubj(std::string subjClass) {
    GameSubj* pGS = NULL;
    if (subjClass.compare("") == 0)
        pGS = (new GameSubj());
    else if (subjClass.find("Car") == 0) {
        if (subjClass.compare("CarWheel") == 0)
            pGS = (new CarWheel());
    }
    if(pGS == NULL) {
        mylog("ERROR in TheGame::newGameSubj. %s class not found\n", subjClass.c_str());
        return NULL;
    }
    myStrcpy_s(pGS->className, 32, subjClass.c_str());
    return pGS;
}
