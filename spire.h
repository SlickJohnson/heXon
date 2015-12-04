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

#ifndef SPIRE_H
#define SPIRE_H

#include <Urho3D/Urho3D.h>

#include "enemy.h"

using namespace Urho3D;

class Spire : public Enemy
{
    URHO3D_OBJECT(Spire, Enemy);
    friend class SpawnMaster;
public:
    Spire(Context* context, MasterControl* masterControl);
    void Hit(float damage, int ownerID);
    void Set(Vector3 position);
protected:
    Node* topNode_;
    Node* bottomNode_;
    StaticModel* topModel_;
    StaticModel* bottomModel_;

    float initialShotInterval_;
    float shotInterval_;
    float sinceLastShot_;

    void HandleSpireUpdate(StringHash eventType, VariantMap &eventData);
};

#endif // SPIRE_H
