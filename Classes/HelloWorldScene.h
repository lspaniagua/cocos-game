/****************************************************************************
 Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.
 
 http://www.cocos2d-x.org
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

#ifndef __HELLOWORLD_SCENE_H__
#define __HELLOWORLD_SCENE_H__

#include "cocos2d.h"

// Map.h
struct Coord {
    int i, j;
    float x, y;
    bool empty, border, obstacle;
    cocos2d::Vec2 points[4];
};

struct Coords {
    Coord * coord;
    Coords * next, * previous;
};

struct Room {
    Coords * beginingCoords, * endingCoords;
    Room * next, * previous;
};

struct Region {
    Room * room;
    Region * next;
};
//

// Controller.h
enum keys { LEFT, UP, RIGHT, DOWN, LENGTH };

struct Nodes {
    cocos2d::Node * node;
    Nodes * next;
};
//

class HelloWorld : public cocos2d::Scene
{
public:
    int width = 0;
    int height = 0;
    Coord ** map = nullptr;
    Coord * currentCoord;
    cocos2d::Camera * defaultCamera;
    cocos2d::Sprite * player;
    static cocos2d::Scene* createScene();
    
    virtual bool init() override;
    virtual void update(float delta) override;
    
    // Map.h
    bool isInsideMap(int i, int j);
    Room * buildRooms(bool empty);
    void validateRooms(Room * beginingRoom, int thresholdSize, bool empty);
    void connectRooms(Room * roomA, Room * roomB, int * bestDistance, bool * possibleConnectionFound, Coord ** bestTileA, Coord ** bestTileB, Room ** bestRoomA, Room ** bestRoomB);
    void buildConnection(Coord * from, Coord * to);
    Coord ** buildMap(int mapWidth, int mapHeight, float tileWidth, float tileHeight, int groundPercent, int obstaclesPercent, Coord ** firstCoord);
    //

    // Controller.h
    float pressedKeys[4] = { .0, .0, .0, .0 };
    Nodes * obstacles = NULL;

    void updateController(float delta, cocos2d::Vec2 * position, Coord * currentCoord);
    void updatePressedKeys(cocos2d::EventKeyboard::KeyCode keyCode, float diff);
    void addObstacle(cocos2d::Node * obstacle);
    void removeObstacle(cocos2d::Node * obstacle);
    //
    
    // implement the "static create()" method manually
    CREATE_FUNC(HelloWorld);
};

#endif // __HELLOWORLD_SCENE_H__
