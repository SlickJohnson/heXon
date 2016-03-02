/* heXon
// Copyright (C) 2016 LucKey Productions (luckeyproductions.nl)
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

#ifndef TILEMASTER_H
#define TILEMASTER_H

#include <Urho3D/Urho3D.h>

#include "mastercontrol.h"

namespace Urho3D {
class Node;
}

class Tile;
class Slot;

enum TileElement {TE_CENTER = 0, TE_NORTH, TE_EAST, TE_SOUTH, TE_WEST, TE_NORTHWEST, TE_NORTHEAST, TE_SOUTHEAST, TE_SOUTHWEST, TE_LENGTH};
enum CornerType {CT_NONE, CT_IN, CT_OUT, CT_TWEEN, CT_DOUBLE, CT_FILL};
enum TileType {B_SPACE, B_EMPTY, B_ENGINE};

class TileMaster : public Object
{
    URHO3D_OBJECT(TileMaster, Object);
    friend class InputMaster;
    friend class MasterControl;
    friend class Tile;
public:
    TileMaster(Context *context, MasterControl* masterControl);

    MasterControl* masterControl_;
    Node* rootNode_;
    RigidBody* rigidBody_;

    void AddToAffectors(WeakPtr<Node> affector, WeakPtr<RigidBody> rigidBody) { hexAffectors_[affector] = rigidBody; }
    void RemoveFromAffectors(WeakPtr<Node> affector) { if (hexAffectors_.Contains(affector) ) hexAffectors_.Erase(affector); }
    HashMap<WeakPtr<Node>, WeakPtr<RigidBody> >* GetAffectors() { return &hexAffectors_; }

    Tile* GetRandomTile();
private:
    Vector3 targetPosition_;
    Vector3 targetScale_;
    Node* logoNode_;
    Material* logoMaterial_;
    Light* playLight_;
    HashMap<WeakPtr<Node>, WeakPtr<RigidBody> > hexAffectors_;
    HashMap<IntVector2, SharedPtr<Tile> > tileMap_;

    void HandleUpdate(StringHash eventType, VariantMap& eventData);

    void EnterPlayState();
    void EnterLobbyState();
};

#endif // TILEMASTER_H
