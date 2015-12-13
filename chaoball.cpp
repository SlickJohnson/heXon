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

#include "spawnmaster.h"
#include "chaoball.h"

ChaoBall::ChaoBall(Context *context, MasterControl *masterControl):
    Pickup(context, masterControl)
{
    rootNode_->SetName("ChaoBall");
    pickupType_ = PT_CHAOBALL;
    rootNode_->SetRotation(Quaternion(Random(360.0f), Random(360.0f), Random(360.0f)));
    initialPosition_ = Vector3::FORWARD*5.0f;
    rootNode_->SetPosition(initialPosition_);
    model_->SetModel(masterControl_->cache_->GetResource<Model>("Resources/Models/Chaosphere.mdl"));
    model_->SetMaterial(masterControl_->cache_->GetTempResource<Material>("Resources/Materials/Chaosphere.xml"));

    rigidBody_->SetMass(3.0f);

    Vector<ColorFrame> colorFrames;
    colorFrames.Push(ColorFrame(Color(0.0f, 1.0f, 0.0f, 0.0f), 0.0f));
    colorFrames.Push(ColorFrame(Color(0.23f, 0.05f, 0.5f, 0.42f), 0.1f));
    colorFrames.Push(ColorFrame(Color(1.0f, 0.0f, 0.0f, 0.0f), 0.4f));
    particleEmitter_->GetEffect()->SetColorFrames(colorFrames);

    Disable();
}
