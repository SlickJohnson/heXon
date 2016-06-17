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

#include "enemy.h"

#include "spawnmaster.h"
#include "player.h"

Enemy::Enemy(Context* context):
    SceneObject(context),
    panicTime_{0.0f},
    initialHealth_{1.0f},
    panic_{0.0f},
    worth_{5},
    bonus_{true},
    firstHitBy_{0},
    lastHitBy_{0},
    whackInterval_{0.5f},
    sinceLastWhack_{0.0f},
    meleeDamage_{0.44f}
{

}

void Enemy::OnNodeSet(Node *node)
{
    SceneObject::OnNodeSet(node);

    node_->SetName("Enemy");

    health_ = initialHealth_;

    //Generate random color
    int randomizer{Random(6)};
    color_ = Color(0.5f + (randomizer * 0.075f), 0.9f - (randomizer * 0.075f), 0.5f+Max(randomizer-3.0f, 3.0f)/6.0f, 1.0f);

    centerNode_ = node_->CreateChild("SmokeTrail");
    particleEmitter_ = centerNode_->CreateComponent<ParticleEmitter>();
    particleEffect_ = CACHE->GetTempResource<ParticleEffect>("Particles/Enemy.xml");
    Vector<ColorFrame> colorFrames{};
    colorFrames.Push(ColorFrame(Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f));
    colorFrames.Push(ColorFrame(Color(color_.r_*0.666f, color_.g_*0.666f, color_.b_*0.666f, 0.5f), 0.1f));
    colorFrames.Push(ColorFrame(Color(0.0f, 0.0f, 0.0f, 0.0f), 1.0f));
    particleEffect_->SetColorFrames(colorFrames);
    particleEmitter_->SetEffect(particleEffect_);

    centerModel_ = centerNode_->CreateComponent<StaticModel>();
    centerModel_->SetModel(MC->GetModel("Core"));
    centerModel_->SetMaterial(MC->GetMaterial("CoreGlow")->Clone());
    centerModel_->GetMaterial(0)->SetShaderParameter("MatDiffColor", color_);
    centerModel_->GetMaterial(0)->SetShaderParameter("MatEmissiveColor", color_);

    rigidBody_ = node_->CreateComponent<RigidBody>();
    rigidBody_->SetRestitution(0.666f);
    rigidBody_->SetLinearDamping(0.1f);
    rigidBody_->SetMass(2.0f);
    rigidBody_->SetLinearFactor(Vector3::ONE - Vector3::UP);
    rigidBody_->SetAngularFactor(Vector3::ZERO);
    CollisionShape* collider{node_->CreateComponent<CollisionShape>()};
    collider->SetSphere(2.0f);
    collider->SetPosition(Vector3::UP * 0.23f);

    for (int s{1}; s <= 5; ++s) {
        samples_.Push(SharedPtr<Sound>(CACHE->GetResource<Sound>("Samples/Melee"+String(s)+".ogg")));
    }
    for (SharedPtr<Sound> s : samples_) {
        s->SetLooped(false);
    }

    Node* soundNode{MC->world.scene->CreateChild("SoundSource")};
    soundSource_ = soundNode->CreateComponent<SoundSource>();
    soundSource_->SetGain(0.1f);
    soundSource_->SetSoundType(SOUND_EFFECT);
}

void Enemy::Set(const Vector3 position)
{
    rigidBody_->SetLinearVelocity(Vector3::ZERO);
    rigidBody_->ResetForces();

    firstHitBy_ = lastHitBy_ = 0;
    bonus_ = true;
    health_ = initialHealth_;
    panic_ = 0.0f;

    particleEmitter_->RemoveAllParticles();
    SceneObject::Set(position);
    MC->tileMaster_->AddToAffectors(WeakPtr<Node>(node_), WeakPtr<RigidBody>(rigidBody_));
    SubscribeToEvent(E_SCENEUPDATE, URHO3D_HANDLER(Enemy, HandleSceneUpdate));
    SubscribeToEvent(node_, E_NODECOLLISIONSTART, URHO3D_HANDLER(Enemy, HandleCollision));
    SubscribeToEvent(node_, E_NODECOLLISION, URHO3D_HANDLER(Enemy, HandleCollision));

    soundSource_->Stop();
}

// Takes care of dealing damage and keeps track of who deserves how many points.
void Enemy::Hit(const float damage, const int playerID) {
    if (firstHitBy_ == 0) firstHitBy_ = playerID;
    else if (firstHitBy_ != playerID && playerID != 0) bonus_ = false;

    lastHitBy_ = playerID;

    SetHealth(health_ - damage);
}

void Enemy::SetHealth(const float health)
{
    health_ = health;
    panic_ = (initialHealth_ - health_) / initialHealth_;
    panic_ < 0.0f ? panic_ = 0.0f : panic_ = panic_;
    particleEffect_->SetMinEmissionRate(7.0f+23.0f*panic_);

    CheckHealth();
}

void Enemy::CheckHealth()
{
    //Die
    if (node_->IsEnabled() && health_ <= 0.0f) {
        if (lastHitBy_ != 0)
            MC->GetPlayer(lastHitBy_)->AddScore(bonus_ ? worth_ : 2 * worth_ / 3);

        MC->spawnMaster_->SpawnExplosion(node_->GetPosition(),
                                                     Color(color_.r_*color_.r_, color_.g_*color_.g_, color_.b_*color_.b_),
                                                     0.5f*rigidBody_->GetMass(),
                                                     lastHitBy_);
        Disable();
    }
}

void Enemy::Disable()
{
    SceneObject::Disable();
}

Color Enemy::GetGlowColor() const
{
    float factor{(Sin(200.0f*(MC->world.scene->GetElapsedTime()+panicTime_))*(0.25f+panic_*0.25f)+(panic_*0.5f))};
    factor *= factor * 2.0f;
    return color_ * factor;
}

void Enemy::HandleSceneUpdate(StringHash eventType, VariantMap &eventData)
{
    float time{MC->world.scene->GetElapsedTime() + node_->GetID() * 0.023f};
    float timeStep{eventData[SceneUpdate::P_TIMESTEP].GetFloat()};
    panicTime_ += 3.0f * panic_ * timeStep;
    sinceLastWhack_ += timeStep;

    Emerge(timeStep);

    //Animate core
    centerModel_->GetMaterial()->SetShaderParameter("VOffset",
                Vector4(0.0f, LucKey::Cycle(time * 3.0f, 0.0f, 1.0f), 0.0f, 0.0f));
    centerNode_->Rotate(Quaternion((1.0f + panic_) * timeStep * 333.0f, Vector3::UP));
}

void Enemy::HandleCollision(StringHash eventType, VariantMap &eventData)
{
    PODVector<RigidBody*> collidingBodies{};
    rigidBody_->GetCollidingBodies(collidingBodies);

    if (sinceLastWhack_ > whackInterval_) {
        for (RigidBody* r : collidingBodies) {
            StringHash otherNameHash = r->GetNode()->GetNameHash();
            if (otherNameHash == N_PLAYER) {
                PlaySample(samples_[Random(static_cast<int>(samples_.Size()))], 0.16f);

                Player* hitPlayer{MC->players_[r->GetNode()->GetID()]};

                hitPlayer->Hit(meleeDamage_ + meleeDamage_*panic_);
                //Spawn hitFX in the middle since collision radii differ for gameplay purposes
                MC->spawnMaster_->SpawnHitFX(
                            (hitPlayer->GetPosition() + GetPosition()) * 0.5f, 0, false);
            }
            sinceLastWhack_ = 0.0f;
        }
    }
}

