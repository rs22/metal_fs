library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.fosi_util.all;

package fosi_ctrl is

  subtype t_Logic is std_logic;
  subtype t_Logic_v is unsigned;

  subtype t_Handshake_ms is std_logic;
  subtype t_Handshake_sm is std_logic;
  subtype t_Handshake_v_ms is unsigned;
  subtype t_Handshake_v_sm is unsigned;

  
  -----------------------------------------------------------------------------
  -- General Axi Declarations
  -----------------------------------------------------------------------------

  -- byte address bits of the boundary a burst may not cross(4KB blocks)
  constant c_AxiBurstAlignWidth : integer := 12;

  constant c_AxiLenWidth    : integer := 8;
  subtype t_AxiLen is unsigned(c_AxiLenWidth-1 downto 0);
  constant c_AxiNullLen : t_AxiLen := (others => '0');

  constant c_AxiSizeWidth   : integer := 3;
  subtype t_AxiSize is unsigned(c_AxiSizeWidth-1 downto 0);

  constant c_AxiBurstWidth  : integer := 2;
  subtype t_AxiBurst is unsigned(c_AxiBurstWidth-1 downto 0);
  constant c_AxiBurstFixed : t_AxiBurst := "00";
  constant c_AxiBurstIncr : t_AxiBurst := "01";
  constant c_AxiBurstWrap : t_AxiBurst := "10";
  constant c_AxiNullBurst : t_AxiBurst := c_AxiBurstFixed;

  constant c_AxiLockWidth  : integer := 2;
  subtype t_AxiLock is unsigned(c_AxiLockWidth-1 downto 0);
  constant c_AxiLockNormal : t_AxiLock := "00";
  constant c_AxiLockExclusive : t_AxiLock := "01";
  constant c_AxiLockLocked : t_AxiLock := "10";
  constant c_AxiNullLock   : t_AxiLock := c_AxiLockNormal;

  constant c_AxiCacheWidth  : integer := 4;
  subtype t_AxiCache is unsigned(c_AxiCacheWidth-1 downto 0);
  constant c_AxiNullCache  : t_AxiCache  := "0010"; -- Normal, NoCache, NoBuffer

  constant c_AxiProtWidth   : integer := 3;
  subtype t_AxiProt is unsigned(c_AxiProtWidth-1 downto 0);
  constant c_AxiNullProt   : t_AxiProt   := "000";  -- Unprivileged, Non-Sec, Data

  constant c_AxiQosWidth    : integer := 4;
  subtype t_AxiQos is unsigned(c_AxiQosWidth-1 downto 0);
  constant c_AxiNullQos    : t_AxiQos    := "0000"; -- No QOS

  constant c_AxiRegionWidth : integer := 4;
  subtype t_AxiRegion is unsigned(c_AxiRegionWidth-1 downto 0);
  constant c_AxiNullRegion : t_AxiRegion := "0000"; -- Default Region

  constant c_AxiRespWidth : integer := 2;
  subtype t_AxiResp is unsigned(c_AxiRespWidth-1 downto 0);
  constant c_AxiRespOkay   : t_AxiResp := "00";
  constant c_AxiRespExOkay : t_AxiResp := "01";
  constant c_AxiRespSlvErr : t_AxiResp := "10";
  constant c_AxiRespDecErr : t_AxiResp := "11";

  -------------------------------------------------------------------------------
  -- Axi Interface: Ctrl
  -------------------------------------------------------------------------------
  --Scalars:

  constant c_CtrlDataWidth : integer := 32;
  subtype t_CtrlData is unsigned (c_CtrlDataWidth-1 downto 0);

  constant c_CtrlStrbWidth : integer := c_CtrlDataWidth/8;
  subtype t_CtrlStrb is unsigned (c_CtrlStrbWidth-1 downto 0);

  constant c_CtrlAddrWidth : integer := 32;
  subtype t_CtrlAddr is unsigned (c_CtrlAddrWidth-1 downto 0);

  constant c_CtrlByteAddrWidth : integer := f_clog2(c_CtrlStrbWidth);
  constant c_CtrlFullSize : t_AxiSize := to_unsigned(c_CtrlByteAddrWidth, t_AxiSize'length);
  subtype t_CtrlByteAddr is unsigned (c_CtrlByteAddrWidth-1 downto 0);

  constant c_CtrlRespWidth : integer := 2;
  subtype t_CtrlResp is unsigned (c_CtrlRespWidth-1 downto 0);


  --Complete Bundle:
  type t_Ctrl_ms is record
    awaddr   : t_CtrlAddr;
    awvalid  : std_logic;
    wdata    : t_CtrlData;
    wstrb    : t_CtrlStrb;
    wvalid   : std_logic;
    bready   : std_logic;
    araddr   : t_CtrlAddr;
    arvalid  : std_logic;
    rready   : std_logic;
  end record;
  type t_Ctrl_sm is record
    awready  : std_logic;
    wready   : std_logic;
    bresp    : t_CtrlResp;
    bvalid   : std_logic;
    arready  : std_logic;
    rdata    : t_CtrlData;
    rresp    : t_CtrlResp;
    rlast    : std_logic;
    rvalid   : std_logic;
  end record;
  constant c_CtrlNull_ms : t_Ctrl_ms := (
    awaddr   => (others => '0'),
    awvalid  => '0',
    wdata    => (others => '0'),
    wstrb    => (others => '0'),
    wvalid   => '0',
    bready   => '0',
    araddr   => (others => '0'),
    arvalid  => '0',
    rready   => '0' );
  constant c_CtrlNull_sm : t_Ctrl_sm := (
    awready  => '0',
    wready   => '0',
    bresp    => (others => '0'),
    bvalid   => '0',
    arready  => '0',
    rdata    => (others => '0'),
    rresp    => (others => '0'),
    rlast    => '0',
    rvalid   => '0' );

end fosi_ctrl;
