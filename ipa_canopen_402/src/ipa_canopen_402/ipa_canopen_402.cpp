#include <ipa_canopen_402/ipa_canopen_402.h>

using namespace ipa_canopen;

void Node_402::read(LayerStatus &status)
{
  std::bitset<16> sw_new(status_word.get());

  status_word_bitset = sw_new;

  if(!status_word_bitset.test(SW_Ready_To_Switch_On))
  {
    if(status_word_bitset.test(SW_Fault))
    {
      state_ = Fault;
    }
    else if(status_word_bitset.test(SW_Switch_on_disabled))
    {
      state_ = Switch_On_Disabled;
    }
    else
      state_ = Not_Ready_To_Switch_On;
  }
  else
  {
    if(status_word_bitset.test(SW_Switched_On))
    {
      if(status_word_bitset.test(SW_Operation_enabled))
      {
        //if(volt_enable)
        // {
        state_ = Operation_Enable;
        // }
      }
      else
        state_ = Switched_On;
    }
    else if(!status_word_bitset.test(SW_Quick_stop))
      state_ = Quick_Stop_Active;

    else
      state_ = Ready_To_Switch_On;
  }

  if(status_word_bitset.test(SW_Fault) & status_word_bitset.test(SW_Operation_enabled) & status_word_bitset.test(SW_Switched_On) & status_word_bitset.test(SW_Ready_To_Switch_On))
    state_ = Fault_Reaction_Active;

  operation_mode_ = op_mode_display.get();
  ac_vel_ = actual_vel.get();
  ac_pos_ = actual_pos.get();
  //internal_pos_ = actual_internal_pos.get();
  oldpos_ = target_position.get_cached();
  status.OK;
  if(check_mode)
  {
    if(operation_mode_ == operation_mode_to_set_)
    {
      check_mode = false;
    }
    else
    {
      status.WARN;
    }
  }

}

void Node_402::shutdown(LayerStatus &status)
{

}

void Node_402::report(LayerStatusExtended &status)
{

}

void Node_402::recover(LayerStatusExtended &status)
{

}

const double Node_402::getTargetPos()
{
  return target_pos_;
}

void Node_402::write(LayerStatus &status)
{
  if (state_ != target_state_)
  {

    if (state_ == Fault)
    {
      control_word_bitset.set(CW_Fault_Reset);
    }
    else
      control_word_bitset.reset(CW_Fault_Reset);

    if (state_ == Not_Ready_To_Switch_On || state_ == Switch_On_Disabled)
    {
      // 0x06 Shutdown
      control_word_bitset.reset(CW_Switch_On);
      control_word_bitset.set(CW_Enable_Voltage);
      control_word_bitset.set(CW_Quick_Stop);
    }

    else if(state_ == Ready_To_Switch_On)
    {
      if (target_state_ == Switch_On_Disabled)
      {
        control_word_bitset.set(CW_Enable_Voltage);
      }
      else
      {
        control_word_bitset.set(CW_Enable_Voltage);
        control_word_bitset.set(CW_Switch_On);
        control_word_bitset.set(CW_Quick_Stop);
      }
    }

    else if (state_ == Switched_On)
    {
      if (target_state_ == Switch_On_Disabled)
      {
        control_word_bitset.reset(CW_Enable_Voltage);
      }
      else if (target_state_ == Ready_To_Switch_On)
      {
        // 0x06 Shutdown
        control_word_bitset.reset(CW_Switch_On);
        control_word_bitset.set(CW_Enable_Voltage);
        control_word_bitset.set(CW_Quick_Stop);
      }
      else
      {
        // 0x07 Enable Operation
        control_word_bitset.set(CW_Switch_On);
        control_word_bitset.set(CW_Enable_Voltage);
        control_word_bitset.set(CW_Enable_Operation);
        control_word_bitset.set(CW_Quick_Stop);
      }
    }
    else if (state_ == Operation_Enable)
    {
      if (target_state_ == Switch_On_Disabled)
      {
        control_word_bitset.reset(CW_Enable_Voltage);
      }
      else if (target_state_ == Ready_To_Switch_On)
      {
        // 0x06 Shutdown
        control_word_bitset.reset(CW_Switch_On);
        control_word_bitset.set(CW_Enable_Voltage);
        control_word_bitset.set(CW_Quick_Stop);
      }
      //Disable operation ?
    }
    int16_t cw_set = static_cast<int>(control_word_bitset.to_ulong());
    control_word.set(cw_set);

    status.WARN;
  }
  else
  {
    status.OK;
    if(operation_mode_ == Profiled_Position)
    {
      if(new_target_pos_)
      {
        control_word.set(0x0f);
        new_target_pos_ = false;
      }
      else if(oldpos_ != target_pos_)
      {
        profile_velocity.set(1000000);
        target_position.set(target_pos_);
        new_target_pos_ = true;
        control_word.set(0x1f);
      }
    }
    else if(operation_mode_ == Profiled_Velocity)
      target_velocity.set(target_vel_);
    }
}

const Node_402::State& Node_402::getState()
{
  return state_;
}

const int8_t Node_402::getMode()
{
  return operation_mode_;
}

const double Node_402::getActualVel()
{
  return ac_vel_;
}

const double Node_402::getActualEff()
{
  return ac_eff_;
}

const double Node_402::getActualPos()
{
  return ac_pos_;
}

const double Node_402::getActualInternalPos()
{
  return internal_pos_;
}

void Node_402::setTargetVel(const double &target_vel)
{
  target_vel_ = target_vel;
}

void Node_402::setTargetPos(const double &target_pos)
{
  target_pos_ = target_pos;
}

bool Node_402::enterMode(const OperationMode &op_mode_var)
{
  op_mode.set(op_mode_var);

  operation_mode_to_set_ = op_mode_var;

  check_mode = true;

  return true;
}

void Node_402::configureEntries()
{
  n_->getStorage()->entry(status_word, 0x6041);
  n_->getStorage()->entry(control_word, 0x6040);

  n_->getStorage()->entry(op_mode,0x6060);
  n_->getStorage()->entry(op_mode_display,0x6061);

  n_->getStorage()->entry(actual_vel,0x606C);
  n_->getStorage()->entry(target_velocity,0x60FF);
  n_->getStorage()->entry(profile_velocity,0x6081);

  n_->getStorage()->entry(target_position,0x607A);
  n_->getStorage()->entry(actual_pos,0x6064);
  n_->getStorage()->entry(actual_internal_pos,0x6063);
}

bool Node_402::turnOn()
{
  target_state_ = Operation_Enable;

  return true;
}

bool Node_402::turnOff()
{
  control_word_buffer.clear();
  control_word_buffer.push_back(0x6);
  return true;
}

void Node_402::init(LayerStatusExtended &s)
{

  enterMode(Profiled_Position);

  if(Node_402::turnOn())
  {
    s.OK;
    running=true;
  }
  else
    s.ERROR;
}