#pragma once

#include <cstdint>
#include <vector>

#pragma pack(push, 1)

struct MoldUDP64Header
{
  char session[10];
  uint16_t message_count;
  uint64_t sequence_number;
};

struct MessageBlock
{
  uint16_t length;
  char type;

  struct NewOrder
  {
    uint32_t timestamp_nanoseconds;
    uint64_t order_id;
    uint32_t orderbook_id;
    char side;
    uint32_t orderbook_position;
    uint64_t quantity;
    int32_t price;
    uint16_t order_attributes;
    uint8_t lot_type;
  };

  struct ExecutionNotice
  {
    uint32_t timestamp_nanoseconds;
    uint64_t order_id;
    uint32_t orderbook_id;
    char side;
    uint64_t executed_quantity;
    uint64_t match_id;
    uint32_t combo_group_id;
    char reserved1[7];
    char reserved2[7];
  };

  struct ExecutionNoticeWithTradeInfo
  {
    uint32_t timestamp_nanoseconds;
    uint64_t order_id;
    uint32_t orderbook_id;
    char side;
    uint64_t executed_quantity;
    uint64_t match_id;
    uint32_t combo_group_id;
    char reserved1[7];
    char reserved2[7];
    uint32_t trade_price;
    char occurred_cross;
    char printable;
  };

  struct DeletedOrder
  {
    uint32_t timestamp_nanoseconds;
    uint64_t order_id;
    uint32_t orderbook_id;
    char side;
  };

  union
  {
    NewOrder new_order;
    ExecutionNotice execution_notice;
    ExecutionNoticeWithTradeInfo execution_notice_with_trade_info;
    DeletedOrder order_delete;
  } data;
};

#pragma pack(pop)