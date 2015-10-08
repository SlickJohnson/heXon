/* heXon
// Copyright (C) 2015 LucKey Productions (luckeyproductions.nl)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <fstream>

#include "hexocam.h"
#include "inputmaster.h"
#include "spawnmaster.h"
#include "player.h"
#include "apple.h"
#include "heart.h"
#include "multix.h"
#include "chaoball.h"
#include "bullet.h"
#include "seeker.h"
#include "flash.h"
#include "hitfx.h"
#include "explosion.h"
#include "muzzle.h"
#include "tailgenerator.h"
#include "chaomine.h"
#include "mastercontrol.h"

DEFINE_APPLICATION_MAIN(MasterControl);

MasterControl::MasterControl(Context *context):
    Application(context),
    currentState_{GS_INTRO},
    paused_{false},
    editMode_{false}
{
}

void MasterControl::Setup()
{
    // Modify engine startup parameters.
    //Set custom window title and icon.
    engineParameters_["WindowTitle"] = "heXon";
    engineParameters_["LogName"] = GetSubsystem<FileSystem>()->GetAppPreferencesDir("urho3d", "logs")+"heXon.log";
//    engineParameters_["VSync"] = true;
//    engineParameters_["FullScreen"] = false;
//    engineParameters_["Headless"] = false;
//    engineParameters_["WindowWidth"] = 1600;
//    engineParameters_["WindowHeight"] = 900;
    //engineParameters_["RenderPath"] = "RenderPaths/DeferredHWDepth.xml";
}
void MasterControl::Start()
{
    TailGenerator::RegisterObject(context_);

    new InputMaster(context_, this);
    cache_ = GetSubsystem<ResourceCache>();
    graphics_ = GetSubsystem<Graphics>();
    renderer_ = GetSubsystem<Renderer>();
    CreateSineLookupTable();

    LoadResources();

    // Get default style
    defaultStyle_ = cache_->GetResource<XMLFile>("UI/DefaultStyle.xml");
    //Create console and debug HUD.
    CreateConsoleAndDebugHud();
    //Create the scene content
    CreateScene();
    //Create the UI content
    CreateUI();
    //Hook up to the frame update and render post-update events
    SubscribeToEvents();

    menuMusic_ = cache_->GetResource<Sound>("Resources/Music/Eddy J - Webbed Gloves in Neon Brights.ogg");
    menuMusic_->SetLooped(true);
    gameMusic_ = cache_->GetResource<Sound>("Resources/Music/Alien Chaos - Disorder.ogg");
    gameMusic_->SetLooped(true);
    Node* musicNode = world.scene->CreateChild("Music");
    musicSource_ = musicNode->CreateComponent<SoundSource>();
    musicSource_->SetGain(0.32f);
    musicSource_->SetSoundType(SOUND_MUSIC);
    musicSource_->Play(menuMusic_);

    SetGameState(GS_LOBBY);
}
void MasterControl::Stop()
{
    engine_->DumpResources(true);
}

void MasterControl::SubscribeToEvents()
{
    //Subscribe scene update event.
    SubscribeToEvent(E_SCENEUPDATE, HANDLER(MasterControl, HandleSceneUpdate));
}

void MasterControl::CreateConsoleAndDebugHud()
{
    // Create console
    Console* console = engine_->CreateConsole();
    console->SetDefaultStyle(defaultStyle_);
    console->GetBackground()->SetOpacity(0.8f);

    // Create debug HUD.
    DebugHud* debugHud = engine_->CreateDebugHud();
    debugHud->SetDefaultStyle(defaultStyle_);
}

void MasterControl::CreateUI()
{
    ui_ = GetSubsystem<UI>();

    //Create a Cursor UI element because we want to be able to hide and show it at will. When hidden, the mouse cursor will control the camera
    world.cursor.uiCursor = new Cursor(context_);
    world.cursor.uiCursor->SetVisible(true);
    ui_->SetCursor(world.cursor.uiCursor);

    //Set starting position of the cursor at the rendering window center
    world.cursor.uiCursor->SetPosition(graphics_->GetWidth()/2, graphics_->GetHeight()/2);
}

void MasterControl::LoadResources()
{
    //Load models
    resources.models.pilots.male = cache_->GetResource<Model>("Resources/Models/Male.mdl");
    resources.models.pilots.female = cache_->GetResource<Model>("Resources/Models/Female.mdl");

    resources.models.ships.swift = cache_->GetResource<Model>("Resources/Models/KlåMk10.mdl");

    resources.models.arenaElements.backgroundTile = cache_->GetResource<Model>("Resources/Models/BackgroundTile.mdl");
    resources.models.arenaElements.obstacle = cache_->GetResource<Model>("Resources/Models/Obstacle.mdl");

    //Load materials
    resources.materials.skin.Push(SharedPtr<Material>(cache_->GetResource<Material>("Resources/Materials/Skin_0.xml")));
    resources.materials.skin.Push(SharedPtr<Material>(cache_->GetResource<Material>("Resources/Materials/Skin_1.xml")));
    resources.materials.skin.Push(SharedPtr<Material>(cache_->GetResource<Material>("Resources/Materials/Skin_2.xml")));
    resources.materials.skin.Push(SharedPtr<Material>(cache_->GetResource<Material>("Resources/Materials/Skin_3.xml")));
    resources.materials.skin.Push(SharedPtr<Material>(cache_->GetResource<Material>("Resources/Materials/Skin_4.xml")));

    resources.materials.cloth.Push(SharedPtr<Material>(cache_->GetResource<Material>("Resources/Materials/ClothWhite.xml")));
    resources.materials.cloth.Push(SharedPtr<Material>(cache_->GetResource<Material>("Resources/Materials/ClothBlack.xml")));
    resources.materials.cloth.Push(SharedPtr<Material>(cache_->GetResource<Material>("Resources/Materials/ClothRed.xml")));
    resources.materials.cloth.Push(SharedPtr<Material>(cache_->GetResource<Material>("Resources/Materials/ClothYellow.xml")));
    resources.materials.cloth.Push(SharedPtr<Material>(cache_->GetResource<Material>("Resources/Materials/ClothGreen.xml")));
    resources.materials.cloth.Push(SharedPtr<Material>(cache_->GetResource<Material>("Resources/Materials/ClothBlue.xml")));

    resources.materials.shoes.Push(SharedPtr<Material>(cache_->GetResource<Material>("Resources/Materials/ClothBlue.xml")));

    resources.materials.hair.Push(SharedPtr<Material>(cache_->GetResource<Material>("Resources/Materials/ClothBlue.xml")));

    resources.materials.shipPrimary = cache_->GetResource<Material>("Resources/Materials/GreenEnvmap.xml");
    resources.materials.shipSecondary = cache_->GetResource<Material>("Resources/Materials/GreenGlowEnvmap.xml");
}

void MasterControl::CreateScene()
{
    world.scene = new Scene(context_);

    world.octree = world.scene->CreateComponent<Octree>();
    physicsWorld_ = world.scene->CreateComponent<PhysicsWorld>();
    physicsWorld_->SetGravity(Vector3::ZERO);
    world.scene->CreateComponent<DebugRenderer>();

    //Create a Zone component for ambient ing & fog control
    Node* zoneNode = world.scene->CreateChild("Zone");
    Zone* zone = zoneNode->CreateComponent<Zone>();
    zone->SetBoundingBox(BoundingBox(Vector3(-100.0f, -50.0f, -100.0f),Vector3(100.0f, 5.0f, 100.0f)));
    zone->SetAmbientColor(Color(0.023f, 0.01f, 0.05f));
    zone->SetFogColor(Color(0.0f, 0.0f, 0.0f));
    zone->SetFogStart(56.8f);
    zone->SetFogEnd(61.8f);

    //Create cursor
    world.cursor.sceneCursor = world.scene->CreateChild("Cursor");
    //world.cursor.sceneCursor->SetPosition(Vector3(0.0f,0.0f,0.0f));
    StaticModel* cursorObject = world.cursor.sceneCursor->CreateComponent<StaticModel>();
    cursorObject->SetModel(cache_->GetResource<Model>("Resources/Models/Hexagon.mdl"));
    cursorObject->SetMaterial(cache_->GetResource<Material>("Resources/Materials/Glow.xml"));
    world.cursor.sceneCursor->SetEnabled(false);

    //Create an invisible plane for mouse raycasting
    world.voidNode = world.scene->CreateChild("Void");
    //Location is set in update since the plane moves with the camera.
    world.voidNode->SetScale(Vector3(1000.0f, 1.0f, 1000.0f));
    StaticModel* planeObject = world.voidNode->CreateComponent<StaticModel>();
    planeObject->SetModel(cache_->GetResource<Model>("Models/Plane.mdl"));
    planeObject->SetMaterial(cache_->GetResource<Material>("Resources/Materials/Invisible.xml"));

    //Create camera
    world.camera = new heXoCam(context_, this);

    //Create arena
    tileMaster_ = new TileMaster(context_, this);
    /*for (int i = 0; i < 6; i++){
        new ArenaEdge(context_, this, (60.0f * i)+30.0f);
    }*/

    //Create heXon logo
    Node* logoNode = world.scene->CreateChild("heXon");
    logoNode->SetWorldPosition(Vector3(0.0f, -4.0f, 0.0f));
    logoNode->SetRotation(Quaternion(0.0f, 180.0f, 0.0f));
    logoNode->SetScale(16.0f);
    StaticModel* logoModel = logoNode->CreateComponent<StaticModel>();
    logoModel->SetModel(cache_->GetResource<Model>("Resources/Models/heXon.mdl"));
    logoModel->SetMaterial(cache_->GetResource<Material>("Resources/Materials/Loglow.xml"));

    //Construct lobby
    lobbyNode_ = world.scene->CreateChild("Lobby");
    lobbyNode_->Rotate(Quaternion(0.0f, 180.0f, 0.0f));
    Node* floorNode = lobbyNode_->CreateChild("Floor");
    floorNode->SetPosition(Vector3(0.0f, -0.5f, 0.0f));
    floorNode->SetScale(10.0f);
    floorNode->Rotate(Quaternion(0.0f, 30.0f, 0.0f));
    StaticModel* floorModel = floorNode->CreateComponent<StaticModel>();
    floorModel->SetModel(cache_->GetResource<Model>("Resources/Models/Hexagon.mdl"));
    floorModel->SetMaterial(cache_->GetResource<Material>("Resources/Materials/Black.xml"));
    floorModel->SetCastShadows(true);
    //Create central ship
    StaticModel* ship = lobbyNode_->CreateChild("Ship")->CreateComponent<StaticModel>();
    ship->SetModel(resources.models.ships.swift);
    ship->SetMaterial(0, resources.materials.shipSecondary /*cache_->GetResource<Material>("Resources/Materials/GreenGlow.xml")*/);
    ship->SetMaterial(1, resources.materials.shipPrimary /*cache_->GetResource<Material>("Resources/Materials/Green.xml")*/);
    ship->SetCastShadows(true);
    RigidBody* lobbyBody = lobbyNode_->CreateComponent<RigidBody>();
    lobbyBody->SetTrigger(true);
    CollisionShape* shipShape = lobbyNode_->CreateComponent<CollisionShape>();
    shipShape->SetCylinder(1.8f, 1.0f);

    SubscribeToEvent(lobbyNode_, E_NODECOLLISIONSTART, HANDLER(MasterControl, HandlePlayTrigger));

    //Add a point light to the lobby. Enable cascaded shadows on it
    Node* lobbySpotLightNode = lobbyNode_->CreateChild("PointLight");
    lobbySpotLightNode->SetPosition(Vector3::UP*5.0f);
    lobbySpotLightNode->SetRotation(Quaternion(90.0f, 0.0f, 0.0f));
    lobbySpotLight_ = lobbySpotLightNode->CreateComponent<Light>();
    lobbySpotLight_->SetLightType(LIGHT_SPOT);
    lobbySpotLight_->SetFov(120.0f);
    lobbySpotLight_->SetBrightness(1.0f);
    lobbySpotLight_->SetRange(10.0f);
    lobbySpotLight_->SetColor(Color(0.3f, 0.5f, 1.0f));
    lobbySpotLight_->SetCastShadows(true);
    lobbySpotLight_->SetShadowBias(BiasParameters(0.0001f, 0.1f));

    for (int i = 0; i < 6; i++){
        Node* edgeNode = floorNode->CreateChild("LobbyEdge");
        edgeNode->Rotate(Quaternion(0.0f, (60.0f * i), 0.0f));
        Model* model = cache_->GetResource<Model>("Resources/Models/ArenaEdgeSegment.mdl");
        edgeNode->CreateComponent<RigidBody>();
        CollisionShape* collider = edgeNode->CreateComponent<CollisionShape>();
        collider->SetConvexHull(model);
        SubscribeToEvent(edgeNode, E_NODECOLLISIONSTART, HANDLER(MasterControl, HandleExitTrigger));
    }
    //Create game elements
    spawnMaster_ = new SpawnMaster(context_, this);
    player_ = new Player(context_, this);
    apple_ = new Apple(context_, this);
    heart_ = new Heart(context_, this);
    multiX_ = new MultiX(context_, this);
    chaoBall_ = new ChaoBall(context_, this);
}

void MasterControl::SetGameState(GameState newState)
{
    if (newState != currentState_){
        LeaveGameState();
        currentState_ = newState;
        EnterGameState();
    }
}
void MasterControl::LeaveGameState()
{
    switch (currentState_){
    case GS_INTRO : break;
    case GS_LOBBY : {
        lobbyNode_->SetEnabledRecursive(false);
    } break;
    case GS_PLAY : {
        spawnMaster_->Deactivate();
    } break; //Eject when alive
    case GS_DEAD : {
        player_->CreateNewPilot();
        player_->ResetScore();
        world.camera->SetGreyScale(false);
    } break;
    case GS_EDIT : break; //Disable EditMaster
    default: break;
    }
}
void MasterControl::EnterGameState()
{
    switch (currentState_){
    case GS_INTRO : break;
    case GS_LOBBY : {
        musicSource_->Play(menuMusic_);
        lobbyNode_->SetEnabledRecursive(true);
        player_->EnterLobby();
        world.camera->EnterLobby();
        spawnMaster_->Clear();
        tileMaster_->HideArena();
        apple_->Deactivate();
        heart_->Deactivate();
        multiX_->Deactivate();
        chaoBall_->Deactivate();
    } break;
    case GS_PLAY : {
        musicSource_->Play(gameMusic_);
        player_->EnterPlay();
        world.camera->EnterPlay();
        apple_->Respawn(true);
        heart_->Respawn(true);
        world.lastReset = world.scene->GetElapsedTime();
        spawnMaster_->Restart();
        tileMaster_->Restart();
    } break;
    case GS_DEAD : {
        spawnMaster_->Deactivate();
        world.camera->SetGreyScale(true);
    } break;
    case GS_EDIT : break; //Activate EditMaster
        default: break;
    }
}

void MasterControl::Eject()
{
    SetGameState(GS_LOBBY);
}

void MasterControl::HandleSceneUpdate(StringHash eventType, VariantMap &eventData)
{
    using namespace Update;
    double timeStep = eventData[P_TIMESTEP].GetFloat();
    UpdateCursor(timeStep);

    if (currentState_ == GS_LOBBY) {
        Color spotColor;
        unsigned score = player_->GetScore();
        spotColor.FromHSV(heXon::Cycle(((score/20000)+0.1f)*world.scene->GetElapsedTime(), 0.0f, 1.0f), Min(1.0f, score/5000), 1.0f);
        lobbySpotLight_->SetColor(spotColor);
        lobbySpotLight_->SetBrightness(1.0f + Min(10.0f, (score/10000)));
    }
}

void MasterControl::UpdateCursor(double timeStep)
{
    world.cursor.sceneCursor->Rotate(Quaternion(0.0f,100.0f*timeStep,0.0f));
    //world.cursor.sceneCursor->SetScale((world.cursor.sceneCursor->GetWorldPosition() - world.camera->GetWorldPosition()).Length()*0.05f);
    if (CursorRayCast(250.0f, world.cursor.hitResults))
    {
        for (unsigned i = 0; i < world.cursor.hitResults.Size(); i++)
        {
            if (world.cursor.hitResults[i].node_->GetNameHash() == N_TILE)
            {
                world.cursor.sceneCursor->SetWorldPosition(world.cursor.hitResults[i].node_->GetPosition()+Vector3::UP);
                /*Vector3 camHitDifference = world.camera->rootNode_->GetWorldPosition() - world.cursor.hitResults[i].position_;
                camHitDifference /= world.camera->rootNode_->GetWorldPosition().y_ - world.voidNode->GetPosition().y_;
                camHitDifference *= world.camera->rootNode_->GetWorldPosition().y_;
                world.cursor.sceneCursor->SetWorldPosition(world.camera->rootNode_->GetWorldPosition()-camHitDifference);*/
            }
        }
    }
}

bool MasterControl::CursorRayCast(double maxDistance, PODVector<RayQueryResult> &hitResults)
{
    IntVector2 mousePos = world.cursor.uiCursor->GetPosition();
    Ray cameraRay = world.camera->camera_->GetScreenRay((float)mousePos.x_/graphics_->GetWidth(), (float)mousePos.y_/graphics_->GetHeight());
    RayOctreeQuery query(hitResults, cameraRay, RAY_TRIANGLE, maxDistance, DRAWABLE_GEOMETRY);
    world.scene->GetComponent<Octree>()->Raycast(query);
    if (hitResults.Size()) return true;
    else return false;
}

bool MasterControl::PhysicsRayCast(PODVector<PhysicsRaycastResult> &hitResults, Ray ray, float distance, unsigned collisionMask = M_MAX_UNSIGNED)
{
    if (distance > 1.0e-9)
        physicsWorld_->Raycast(hitResults, ray, distance, collisionMask);

    return (hitResults.Size() > 0);
}

bool MasterControl::PhysicsSphereCast(PODVector<RigidBody*> &hitResults, Vector3 center, float radius, unsigned collisionMask = M_MAX_UNSIGNED)
{
    physicsWorld_->GetRigidBodies(hitResults, Sphere(center, radius), collisionMask);
    if (hitResults.Size()) return true;
    else return false;
}

void MasterControl::Exit()
{
    //Save score to file
    std::ofstream fScore;
    fScore.open ("Resources/.score.zip");
    fScore << player_->GetScore();
    fScore.close();
    ///Save pilot to file

    //And exit
    engine_->Exit();

}

void MasterControl::CreateSineLookupTable()
{
    //Populate sine lookup vector
    int resolution = 4096;
    for (int i = 0; i < resolution; i++){
        sine_.Push(sin(((float)i/resolution)*2.0*M_PI));
    }
}

float MasterControl::Sine(float freq, float min, float max, float shift)
{
    float phase = freq * world.scene->GetElapsedTime() + shift;
    float add = 0.5f*(min+max);
    return Sine(phase) * 0.5f * (max - min) + add;
}
