#include "HelloWorldScene.h"

USING_NS_CC;

Scene* HelloWorld::createScene()
{
    return HelloWorld::create();
}

bool HelloWorld::init()
{
    if ( !Scene::init() )
    {
        return false;
    }

    Director * director = Director::getInstance();
    Size visibleSize = director->getVisibleSize();
    Vec2 origin = director->getVisibleOrigin();
    
    FileUtils * fileUtils = FileUtils::getInstance();
    ValueMap levels = fileUtils->getValueMapFromFile("Levels.plist");
    this->width = levels.at("width").asInt();
    this->height = levels.at("height").asInt();
    float seed = levels.at("seed").asFloat();
    ValueMap ground = levels.at("ground").asValueMap();
    int groundPercent = ground.at("percent").asInt();
    ValueMap obstacles = levels.at("obstacles").asValueMap();
    int obstaclesPercent = obstacles.at("percent").asInt();
    int scale = 1;
    float tileWidth = scale * 30.5;
    float tileHeight = scale * 21.8;
    
    srand(seed);
        
    SpriteFrameCache * spriteCache = SpriteFrameCache::getInstance();
    spriteCache->addSpriteFramesWithFile("ground.plist");
    Size tileSize = Sprite::createWithSpriteFrameName("ground1")->getContentSize();
    currentCoord = NULL;
    this->map = buildMap(this->width, this->height, scale * tileSize.width, scale * tileSize.height, groundPercent, obstaclesPercent, &currentCoord);
    
    // Drawing
    auto rectNode = DrawNode::create();
    Color4F white(1, 1, 1, 1);
    Color4F alpha(1, 1, 1, 0);
    for (int i = 0; i < this->width; i++) {
        for (int j = this->height - 1; j >= 0; j--) {
            Coord tile = this->map[i][j];
            if (tile.empty) {
                continue;
            }
            Sprite * sprite = Sprite::createWithSpriteFrameName("ground1");
            sprite->setAnchorPoint(Vec2::ZERO);
            sprite->setScale(scale);
            sprite->setPosition(Vec2(tile.x, tile.y));
            this->addChild(sprite);
            
            rectNode->drawPolygon(tile.points, 4, alpha, 0.1, white);
            
        }
    }
    this->addChild(rectNode);
    //
    
    // Set camera
    Vec2 firstPosition = Vec2(currentCoord->x, currentCoord->y);
    this->defaultCamera = this->getDefaultCamera();
    this->defaultCamera->setPosition(firstPosition);
    //
    
    //Set player
    spriteCache->addSpriteFramesWithFile("Player.plist");
    
    Vector<SpriteFrame *> * idleSpriteFrames = new Vector<SpriteFrame *>();
    idleSpriteFrames->pushBack(spriteCache->getSpriteFrameByName("down-idle1"));
    idleSpriteFrames->pushBack(spriteCache->getSpriteFrameByName("down-idle2"));
    idleSpriteFrames->pushBack(spriteCache->getSpriteFrameByName("down-idle3"));
    idleSpriteFrames->pushBack(spriteCache->getSpriteFrameByName("down-idle2"));
    Animation * idleAnimation = Animation::createWithSpriteFrames(* idleSpriteFrames, .3f);
    
    player = Sprite::createWithSpriteFrame(idleSpriteFrames->front());
    player->setAnchorPoint(Vec2::ZERO);
    player->setScale(scale);
    player->setPosition(firstPosition);
    player->runAction(RepeatForever::create(Animate::create(idleAnimation)));
    this->addChild(player);
    //

    // Set keyboard events
    EventDispatcher * eventDispatcher = this->getEventDispatcher();
    EventListenerKeyboard * KeyboardListener = EventListenerKeyboard::create();
    KeyboardListener->onKeyPressed = [this](EventKeyboard::KeyCode keyCode, Event * event) {
        updatePressedKeys(keyCode, 1);
    };
    KeyboardListener->onKeyReleased = [this](EventKeyboard::KeyCode keyCode, Event * event) {
        updatePressedKeys(keyCode, 0);
    };
    eventDispatcher->addEventListenerWithSceneGraphPriority(KeyboardListener, this);
    //
    
    this->scheduleUpdate();
    
    return true;
}

void HelloWorld::update(float delta) {
    Vec2 currentPosition = this->defaultCamera->getPosition();
    updateController(delta, &currentPosition, currentCoord);
    this->defaultCamera->setPosition(currentPosition.x, currentPosition.y);
    player->setPosition(currentPosition.x, currentPosition.y);
}

// Map.c
bool HelloWorld::isInsideMap(int i, int j)
{
    return i >= 0 && i < width && j >= 0 && j < height;
}

Room * HelloWorld::buildRooms(bool empty)
{
    bool checkedTiles[width][height];
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            checkedTiles[i][j] = false;
        }
        
    }
    Room * beginingRoom = nullptr;
    Room * endingRoom = nullptr;
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            Coord * tile = &map[i][j];
            if (tile->empty != empty || checkedTiles[i][j]) {
                continue;
            }
            checkedTiles[i][j] = true;
            Coords * coords = (Coords *) malloc(sizeof(Coords));
            coords->coord = tile;
            coords->next = nullptr;
            Room * room = (Room *) malloc(sizeof(Room));
            room->beginingCoords = coords;
            room->endingCoords = coords;
            room->next = nullptr;
            room->previous = endingRoom;
            if (beginingRoom == nullptr) {
                beginingRoom = room;
            }
            if (endingRoom != nullptr) {
                endingRoom->next = room;
            }
            endingRoom = room;
            for (Coords * _coords = room->beginingCoords; _coords != nullptr; _coords = _coords->next) {
                Coord * _tile = _coords->coord;
                int _i = _tile->i;
                int _j = _tile->j;
                for (int ni = _i - 1; ni <= _i + 1; ni++) {
                    for (int nj = _j - 1; nj <= _j + 1; nj++) {
                        if (isInsideMap(ni, nj) && !checkedTiles[ni][nj]) {
                            Coord * neightbor = &map[ni][nj];
                            if (neightbor->empty == empty) {
                                checkedTiles[ni][nj] = true;
                                Coords * nextCoords = (Coords *) malloc(sizeof(Coords));
                                nextCoords->coord = neightbor;
                                nextCoords->next = nullptr;
                                room->endingCoords->next = nextCoords;
                                room->endingCoords = nextCoords;
                            } else {
                                _tile->border = true;
                            }
                        }
                    }
                }
            }
        }
    }
    return beginingRoom;
}

void HelloWorld::validateRooms(Room * beginingRoom, int thresholdSize, bool empty)
{
    for (Room * room = beginingRoom; room != nullptr; ) {
        int size = 0;
        for (Coords * coords = room->beginingCoords; coords != nullptr; coords = coords->next, size++);
        if (size >= thresholdSize) {
            room = room->next;
            continue;
        }
        Room * _room = room;
        for (Coords * coords = _room->beginingCoords; coords != nullptr; coords = coords->next) {
            coords->coord->empty = !empty;
        }
        room = _room->previous;
        if (room == nullptr) {
            room = _room->next;
            room->previous = nullptr;
        }
        else {
            room->next = _room->next;
            if (room->next != nullptr) {
                room->next->previous = room;
            }
        }
        _room->next = nullptr;
        _room->previous = nullptr;
        free(_room);
    }
}

void HelloWorld::connectRooms(Room * roomA, Room * roomB, int * bestDistance, bool * connectionFound, Coord ** bestTileA, Coord ** bestTileB, Room ** bestRoomA, Room ** bestRoomB)
{
    for (Coords * coordsA = roomA->beginingCoords; coordsA != nullptr; coordsA = coordsA->next) {
        Coord * tileA = coordsA->coord;
        if (!tileA->border) {
            continue;
        }
        for (Coords * coordsB = roomB->beginingCoords; coordsB != nullptr; coordsB = coordsB->next) {
            Coord * tileB = coordsB->coord;
            if (!tileB->border) {
                continue;
            }
            int distance = pow(tileA->x - tileB->x, 2) + pow(tileA->y - tileB->y, 2);
            if (distance < * bestDistance || !* connectionFound) {
                * connectionFound = true;
                * bestDistance = distance;
                * bestTileA = tileA;
                * bestTileB = tileB;
                * bestRoomA = roomA;
                * bestRoomB = roomB;
            }
        }
    }
}

void HelloWorld::buildConnection(Coord * from, Coord * to)
{
    int dx = to->i - from->i;
    int dy = to->j - from->j;
    int dxSign = (dx > 0) ? 1 : ((dx < 0) ? -1 : 0);
    int dySign = (dy > 0) ? 1 : ((dy < 0) ? -1 : 0);
    int dxAbs = abs(dx);
    int dyAbs = abs(dy);
    int longest = dxAbs;
    int shortest = dyAbs;
    bool inverted = longest < shortest;
    if (inverted) {
        longest = dyAbs;
        shortest = dxAbs;
    }
    int gradientAccumulation = longest / 2;
    int radious = 1;
    int radiousPow = radious * radious;
    for (int i = 0, _i = from->i, _j = from->j; i < longest; i++) {
        for (int j = -radious; j <= radious; j++) {
            int jPow = j * j;
            for (int k = -radious; k <= radious; k++) {
                if (jPow + k * k <= radiousPow) {
                    int __i = _i + j;
                    int __j = _j + k;
                    if (isInsideMap(__i, __j)) {
                        map[__i][__j].empty = false;
                    }
                }
            }
        }
        if (inverted) {
            _j += dySign;
        }
        else {
            _i += dxSign;
        }
        gradientAccumulation += shortest;
        if (gradientAccumulation >= longest) {
            if (inverted) {
                _i += dxSign;
            }
            else {
                _j += dySign;
            }
            gradientAccumulation -= longest;
        }
    }
}

Coord ** HelloWorld::buildMap(int mapWidth, int mapHeight, float tileWidth, float tileHeight, int groundPercent, int obstaclesPercent, Coord ** firstCoord)
{
    width = mapWidth;
    height = mapHeight;
    
    // Generate map
    map = new Coord * [width];
    float xOffset = 0.64;
    float yOffset = 0.325;
    for (int i = 0, x = 0; i < width; i++, x += tileWidth / 2) {
        map[i] = new Coord [height];
        float iOffset = i * tileHeight * yOffset;
        for (int j = 0, y = 0; j < height; j++, y += tileHeight) {
            Coord * coord = (Coord *) malloc(sizeof(Coord));
            coord->i = i;
            coord->j = j;
            coord->x = x + y * xOffset;
            coord->y = y * yOffset - iOffset;
            coord->empty = i == 0 || i == width - 1 || j == 0 || j == height - 1 || ((rand() % 100) + 1) >= groundPercent;
            coord->border = false;
            coord->obstacle = false;
            coord->points[0] = cocos2d::Vec2(coord->x + 0.8, coord->y + tileHeight * xOffset);
            coord->points[1] = cocos2d::Vec2(coord->x + tileWidth / 2, coord->y + tileHeight - 0.8);
            coord->points[2] = cocos2d::Vec2(coord->x + tileWidth - 0.8, coord->y + tileHeight * xOffset);
            coord->points[3] = cocos2d::Vec2(coord->x + tileWidth / 2, (coord->y + tileHeight * xOffset / 2) + 0);
            map[i][j] = * coord;
        }
    }
    //
    
    // Smooth map
    for (int k = 0; k < 5; k++) {
        for (int i = 0; i < width; i++) {
            for (int j = 0; j < height; j++) {
                Coord * tile = & map[i][j];
                
                // Get empty neighbours
                int emptyNeighbours = 0;
                for (int ni = i - 1; ni <= i + 1; ni++) {
                    for (int nj = j - 1; nj <= j + 1; nj++) {
                        if (isInsideMap(ni, nj)) {
                            if ((ni != i || nj != j) && map[ni][nj].empty) {
                                emptyNeighbours++;
                            }
                        } else {
                            emptyNeighbours++;
                        }
                    }
                }
                //

                if (emptyNeighbours > 4) {
                    tile->empty = true;
                } else if (emptyNeighbours < 4) {
                    tile->empty = false;
                }
            }
        }
    }
    //
    
    //Getting rooms
    Room * beginingFilledRoom = buildRooms(false);
    validateRooms(beginingFilledRoom, 20, false);
    validateRooms(buildRooms(true), 50, true);
    //
    
    //Getting regions
    Region * beginnigRegion = (Region *) malloc(sizeof(Region));
    beginnigRegion->room = beginingFilledRoom;
    beginnigRegion->next = nullptr;
    beginingFilledRoom = beginingFilledRoom->next;
    beginingFilledRoom->previous = nullptr;
    beginnigRegion->room->next = nullptr;
    for (Region * regionA = beginnigRegion; regionA != nullptr; regionA = regionA->next) {
        for (Room * roomA = regionA->room; roomA != nullptr; roomA = roomA->next) {
            if (beginingFilledRoom == nullptr) {
                continue;
            }
            int bestDistance = 0;
            bool possibleConnectionFound = false;
            Coord * bestTileA = nullptr, * bestTileB = nullptr;
            Room * bestRoomA = nullptr, * bestRoomB = nullptr;
            for (Room * roomB = beginingFilledRoom; roomB != nullptr; roomB = roomB->next) {
                if (roomA == roomB) {
                    continue;
                }
                connectRooms(roomA, roomB, &bestDistance, &possibleConnectionFound, &bestTileA, &bestTileB, &bestRoomA, &bestRoomB);
            }
            bestRoomA = nullptr;
            for (Region * regionB = beginnigRegion; regionB != nullptr; regionB = regionB->next) {
                for (Room * roomB = regionB->room; roomB != nullptr; roomB = roomB->next) {
                    if (roomA == roomB) {
                        continue;
                    }
                    connectRooms(roomA, roomB, &bestDistance, &possibleConnectionFound, &bestTileA, &bestTileB, &bestRoomA, &bestRoomB);
                }
            }
            if (bestRoomA != nullptr) {
                Region * region = (Region *) malloc(sizeof(Region));
                region->room = beginingFilledRoom;
                region->next = nullptr;
                beginingFilledRoom = beginingFilledRoom->next;
                if (beginingFilledRoom != nullptr) {
                    beginingFilledRoom->previous = nullptr;
                }
                region->room->next = nullptr;
                roomA->next = nullptr;
                regionA->next = region;
                continue;
            }
            if (possibleConnectionFound) {
                roomA->next = bestRoomB;
                if (bestRoomB->previous == nullptr) {
                    beginingFilledRoom = bestRoomB->next;
                    if (beginingFilledRoom != nullptr) {
                        beginingFilledRoom->previous = nullptr;
                    }
                } else {
                    bestRoomB->previous->next = bestRoomB->next;
                }
                if (bestRoomB->next != nullptr) {
                    bestRoomB->next->previous = bestRoomB->previous;
                }
                bestRoomB->previous = roomA;
                bestRoomB->next = nullptr;
                buildConnection(bestTileA, bestTileB);
            }
        }
    }
    //
    
    //Connecting regions
    while (beginnigRegion->next != nullptr) {
        Region * regionA = beginnigRegion;
        int bestDistance = 0;
        bool possibleConnectionFound = false;
        Coord * bestTileA = nullptr, * bestTileB = nullptr;
        Room * bestRoomA = nullptr, * bestRoomB = nullptr;
        for (Region * regionB = beginnigRegion; regionB != nullptr; regionB = regionB->next) {
            if (regionA == regionB) {
                continue;
            }
            for (Room * roomA = regionA->room; roomA != nullptr; roomA = roomA->next) {
                for (Room * roomB = regionB->room; roomB != nullptr; roomB = roomB->next) {
                    if (roomA == roomB) {
                        continue;
                    }
                    connectRooms(roomA, roomB, &bestDistance, &possibleConnectionFound, &bestTileA, &bestTileB, &bestRoomA, &bestRoomB);
                }
            }
        }
        if (possibleConnectionFound) {
            Room * lastRoomA = nullptr;
            for (lastRoomA = regionA->room; lastRoomA->next != nullptr; lastRoomA = lastRoomA->next);
            Region * bestRegionB = nullptr;
            for (Region * regionB = beginnigRegion; regionB != nullptr; regionB = regionB->next) {
                for (Room * roomB = regionB->room; roomB != nullptr; roomB = roomB->next) {
                    if (roomB == bestRoomB) {
                        bestRegionB = regionB;
                    }
                }
            }
            Region * previousRegionB = nullptr;
            for (previousRegionB = beginnigRegion; previousRegionB->next != bestRegionB; previousRegionB = previousRegionB->next);
            lastRoomA->next = bestRegionB->room;
            previousRegionB->next = bestRegionB->next;
            bestRegionB->next = nullptr;
            buildConnection(bestTileA, bestTileB);
        }
    }
    //
    
    //
    int noEmptyCoordsCount = 0;
    Coords * noEmptyCoords = nullptr, * lastFilledCoord = nullptr;
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            Coord * tile = &map[i][j];
            tile->border = false;
            if (!tile->empty) {
                Coords * coords = (Coords *) malloc(sizeof(Coords));
                coords->coord = tile;
                coords->previous = lastFilledCoord;
                if (lastFilledCoord != nullptr) {
                    lastFilledCoord->next = coords;
                    lastFilledCoord = lastFilledCoord->next;
                } else {
                    coords->next = nullptr;
                    noEmptyCoords = coords;
                    lastFilledCoord = coords;
                }
                noEmptyCoordsCount++;
            }
            for (int ni = i - 1; ni <= i + 1; ni++) {
                for (int nj = j - 1; nj <= j + 1; nj++) {
                    if (isInsideMap(ni, nj)) {
                        Coord * neightbor = &map[ni][nj];
                        if (neightbor->empty != tile->empty) {
                            tile->border = true;
                        }
                    }
                }
            }
        }
    }
    //
       
    //Shuffle coords
    int i = noEmptyCoordsCount;
    Coords * shuffleNoEmptyCoords = nullptr, * lastShuffleNoEmptyCoords = nullptr;
    do {
        int randomI = rand() % i;
        Coords * auxCoords = noEmptyCoords;
        for (int j = 0; j < randomI; j++, auxCoords = auxCoords->next);
        if (noEmptyCoords == auxCoords) {
            noEmptyCoords = noEmptyCoords->next;
        }
        if (auxCoords->previous != nullptr) {
            auxCoords->previous->next = auxCoords->next;
        }
        if (auxCoords->next != nullptr) {
            auxCoords->next->previous = auxCoords->previous;
        }
        auxCoords->previous = lastShuffleNoEmptyCoords;
        if (lastShuffleNoEmptyCoords != nullptr) {
            lastShuffleNoEmptyCoords->next = auxCoords;
            lastShuffleNoEmptyCoords = lastShuffleNoEmptyCoords->next;
        } else {
            auxCoords->next = nullptr;
            shuffleNoEmptyCoords = auxCoords;
            lastShuffleNoEmptyCoords = auxCoords;
        }
    } while (--i > 0);
    //
    
    //Adding obstacles
    bool checkedObstacles[width][height];
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            checkedObstacles[i][j] = false;
        }
        
    }
    * firstCoord = shuffleNoEmptyCoords->coord;
    int obstacleCount = noEmptyCoordsCount * obstaclesPercent / 100;
    int mapCount = width * height;
    int c = 0;
    for (int i = 0; i < obstacleCount; i++) {
        Coords * coords = shuffleNoEmptyCoords;
        shuffleNoEmptyCoords = shuffleNoEmptyCoords->next;
        checkedObstacles[coords->coord->i][coords->coord->j] = true;
        c++;
        
        //Map is fully accessible
        bool mapFlags[width][height];
        for (int i = 0; i < width; i++) {
            for (int j = 0; j < height; j++) {
                mapFlags[i][j] = false;
            }
            
        }
        Coords * _coords = (Coords *) malloc(sizeof(Coords));
        _coords->coord = * firstCoord;
        _coords->next = nullptr;
        _coords->previous = nullptr;
        mapFlags[_coords->coord->i][_coords->coord->j] = true;
        int accessibleTileCount = 1;
        Coords * _lastCoords = _coords;
        do {
            for (int _i = -1; _i <= 1; _i++) {
                for (int _j = -1; _j <= 1; _j++) {
                    int neighbourI = _coords->coord->i + _i;
                    int neighbourJ = _coords->coord->j + _j;
                    if (_i == 0 || _j == 0) {
                        if (isInsideMap(neighbourI, neighbourJ) && !map[neighbourI][neighbourJ].empty) {
                            if (!mapFlags[neighbourI][neighbourJ] && !checkedObstacles[neighbourI][neighbourJ]) {
                                Coords * neighbourCoords = (Coords *) malloc(sizeof(Coords));
                                neighbourCoords->coord = & map[neighbourI][neighbourJ];
                                neighbourCoords->next = nullptr;
                                neighbourCoords->previous = _lastCoords;
                                _lastCoords->next = neighbourCoords;
                                _lastCoords = neighbourCoords;
                                mapFlags[neighbourI][neighbourJ] = true;
                                accessibleTileCount++;
                            }
                        }
                    }
                }
            }
            _coords = _coords->next;
            if (_coords != nullptr) {
                free(_coords->previous);
                _coords->previous = nullptr;
            }
        } while (_coords != nullptr);
        //
        
        if (coords->coord != * firstCoord && noEmptyCoordsCount - c == accessibleTileCount) {
            coords->coord->obstacle = true;
        } else {
            checkedObstacles[coords->coord->i][coords->coord->j] = false;
            c--;
        }
        if (shuffleNoEmptyCoords != nullptr) {
            shuffleNoEmptyCoords->previous = nullptr;
        }
        free(coords);
    }
    //
    
    return map;
}
//

// Controler.c
void HelloWorld::updateController(float delta, cocos2d::Vec2 * position, Coord * currentCoord) {
    for (int i = 0; i < keys::LENGTH; i++) {
        switch (i) {
            case keys::LEFT:
                position->x -= pressedKeys[keys::LEFT];
                break;
            case keys::UP:
                position->y += pressedKeys[keys::UP];
                break;
            case keys::RIGHT:
                position->x += pressedKeys[keys::RIGHT];
                break;
            case keys::DOWN:
                position->y -= pressedKeys[keys::DOWN];
                break;
        }
        bool collision = false;
        cocos2d::PhysicsShapeEdgePolygon * polygon = cocos2d::PhysicsShapeEdgePolygon::create(currentCoord->points, 4);
        if (polygon->containsPoint(* position)) {
            printf("aaaaaaa\n");
        }
        for (Nodes * node = obstacles; node != NULL; node = node->next) {

        }
        if (collision) {
            switch (i) {
                case keys::LEFT:
                    position->x += pressedKeys[keys::LEFT];
                    break;
                case keys::UP:
                    position->y -= pressedKeys[keys::UP];
                    break;
                case keys::RIGHT:
                    position->x -= pressedKeys[keys::RIGHT];
                    break;
                case keys::DOWN:
                    position->y += pressedKeys[keys::DOWN];
                    break;
            }
        }
    }
}

void HelloWorld::updatePressedKeys(cocos2d::EventKeyboard::KeyCode keyCode, float diff) {
    switch (keyCode) {
        case cocos2d::EventKeyboard::KeyCode::KEY_LEFT_ARROW:
            pressedKeys[keys::LEFT] = diff;
            break;
        case cocos2d::EventKeyboard::KeyCode::KEY_UP_ARROW:
            pressedKeys[keys::UP] = diff;
            break;
        case cocos2d::EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
            pressedKeys[keys::RIGHT] = diff;
            break;
        case cocos2d::EventKeyboard::KeyCode::KEY_DOWN_ARROW:
            pressedKeys[keys::DOWN] = diff;
            break;
    }
}

void HelloWorld::addObstacle(cocos2d::Node * obstacle) {
    Nodes * nodes = (Nodes *) malloc(sizeof(Nodes));
    nodes->node = obstacle;
    nodes->next = NULL;
    if (obstacles == NULL) {
        obstacles = nodes;
    } else {
        Nodes * lastNode = obstacles;
        for ( ; lastNode->next != NULL; lastNode = lastNode->next);
        lastNode->next = nodes;
    }
}

void HelloWorld::removeObstacle(cocos2d::Node * obstacle) {
    if (obstacles == NULL) {
        return;
    }
    Nodes * targetNode = obstacles;
    Nodes * previousTargetNode = NULL;
    for ( ; targetNode != NULL && targetNode->node != obstacle; previousTargetNode = targetNode, targetNode = targetNode->next);
    if (targetNode != NULL && targetNode->node == obstacle) {
        if (previousTargetNode != NULL) {
            previousTargetNode->next = targetNode->next;
            targetNode->next = NULL;
        }
        else {
            obstacles = NULL;
        }
        free(targetNode);
    }
}
//
