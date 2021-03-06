// $Id: Rw11VirtTerm.hpp 1186 2019-07-12 17:49:59Z mueller $
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2013-2018 by Walter F.J. Mueller <W.F.J.Mueller@gsi.de>
// 
// Revision History: 
// Date         Rev Version  Comment
// 2018-12-15  1083   1.1.2  SetupRcvCallback(): use rval ref and move semantics
// 2018-12-14  1081   1.1.1  use std::function instead of boost
// 2018-12-02  1076   1.1    use unique_ptr for New()
// 2017-04-07   868   1.0.1  Dump(): add detail arg
// 2013-03-06   495   1.0    Initial version
// 2013-02-13   488   0.1    First draft
// ---------------------------------------------------------------------------

/*!
  \brief   Declaration of class Rw11VirtTerm.
*/

#ifndef included_Retro_Rw11VirtTerm
#define included_Retro_Rw11VirtTerm 1

#include <memory>
#include <functional>

#include "Rw11Virt.hpp"

namespace Retro {

  class Rw11VirtTerm : public Rw11Virt {
    public:
      typedef std::function<bool(const uint8_t*, size_t)> rcvcbfo_t;

      explicit      Rw11VirtTerm(Rw11Unit* punit);
                   ~Rw11VirtTerm();

      virtual const std::string& ChannelId() const;

      void          SetupRcvCallback(rcvcbfo_t&& rcvcbfo);
      virtual bool  Snd(const uint8_t* data, size_t count, RerrMsg& emsg) = 0;

      virtual void  Dump(std::ostream& os, int ind=0, const char* text=0,
                         int detail=0) const;

      static std::unique_ptr<Rw11VirtTerm> New(const std::string& url,
                                               Rw11Unit* punit, RerrMsg& emsg);

    // statistics counter indices
      enum stats {
        kStatNVTRcvPoll = Rw11Virt::kDimStat,
        kStatNVTSnd,
        kStatNVTRcvByt,
        kStatNVTSndByt,
        kDimStat
      };    

    protected:
      std::string   fChannelId;             //!< channel id 
      rcvcbfo_t     fRcvCb;                 //!< receive callback fobj
  };
  
} // end namespace Retro

#include "Rw11VirtTerm.ipp"

#endif
