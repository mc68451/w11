// $Id: RtclRw11UnitTM11.hpp 1186 2019-07-12 17:49:59Z mueller $
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2015-2018 by Walter F.J. Mueller <W.F.J.Mueller@gsi.de>
// 
// Revision History: 
// Date         Rev Version  Comment
// 2018-12-07  1078   1.2    use std::shared_ptr instead of boost
// 2017-04-08   870   1.1    inherit from RtclRw11UnitBase
// 2015-05-17   683   1.0    Initial version
// ---------------------------------------------------------------------------


/*!
  \brief   Declaration of class RtclRw11UnitTM11.
*/

#ifndef included_Retro_RtclRw11UnitTM11
#define included_Retro_RtclRw11UnitTM11 1

#include "librw11/Rw11UnitTM11.hpp"
#include "librw11/Rw11CntlTM11.hpp"

#include "RtclRw11UnitTape.hpp"
#include "RtclRw11UnitBase.hpp"

namespace Retro {

  class RtclRw11UnitTM11 : public RtclRw11UnitBase<Rw11UnitTM11,Rw11UnitTape,
                                                   RtclRw11UnitTape> {
    public:
                    RtclRw11UnitTM11(Tcl_Interp* interp,
                                 const std::string& unitcmd,
                                 const std::shared_ptr<Rw11UnitTM11>& spunit);
                   ~RtclRw11UnitTM11();

    protected:
  };
  
} // end namespace Retro

//#include "RtclRw11UnitTM11.ipp"

#endif
