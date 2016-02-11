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

#include "effect.h"

Effect::Effect(Context *context, MasterControl *masterControl):
    SceneObject(context, masterControl),
    age_{0.0f},
    emitTime_{0.1f}
{
    blink_ = false;

    rootNode_->SetName("Effect");
    particleEmitter_ = rootNode_->CreateComponent<ParticleEmitter>();
}

void Effect::HandleSceneUpdate(StringHash eventType, VariantMap &eventData)
{
    float timeStep = eventData[Update::P_TIMESTEP].GetFloat();
    age_ += timeStep;

    ParticleEffect* effect = particleEmitter_->GetEffect();
    if (effect){
        if (age_ > emitTime_ + effect->GetMaxTimeToLive()){
            Disable();
        }
        else if (age_ > emitTime_){
            particleEmitter_->SetEmitting(false);
        }
    }
}

void Effect::Set(const Vector3 position)
{
    SceneObject::Set(position);
    age_ = 0.0f;
    particleEmitter_->RemoveAllParticles();
    particleEmitter_->SetEmitting(true);
    SubscribeToEvent(E_SCENEUPDATE, URHO3D_HANDLER(Effect, HandleSceneUpdate));
}

void Effect::Disable()
{
    UnsubscribeFromEvent(E_SCENEUPDATE);
    SceneObject::Disable();
}
